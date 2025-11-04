const express = require('express');
const puppeteer = require('puppeteer');
const fs = require('fs');
const os = require('os');
const path = require('path');
const app = express();
app.disable('x-powered-by');
app.use(express.json({ limit: '1mb', strict: true }));

const tplPath = path.join(process.cwd(), 'assets', 'agreement_edo.html');
let AGREEMENT_TEMPLATE = '';
try {
  AGREEMENT_TEMPLATE = fs.readFileSync(tplPath, 'utf8');
  console.log(`INFO template.loaded path=${tplPath}`);
} catch (e) {
  console.error(`FATAL cannot read template ${tplPath}: ${e.message}`);
  process.exit(1);
}

app.use((req, res, next) => {
  const start = Date.now();
  res.on('finish', () => {
    console.log(`${new Date().toISOString()} ${req.method} ${req.originalUrl} ${res.statusCode} ${Date.now() - start}ms`);
  });
  next();
});
const short = (id) => (id ? String(id).slice(0, 8) : 'unknown');

function parsePlaceholders(body) {
  if (typeof body !== 'object' || body === null || Array.isArray(body)) {
    throw new Error('payload must be a JSON object');
  }
  const MAX_KEYS = 200;
  const MAX_VAL_LEN = 10 * 1024;
  const safe = Object.create(null);
  let count = 0;
  for (const k of Object.keys(body)) {
    if (k === '__proto__' || k === 'prototype' || k === 'constructor') continue;
    const v = body[k];
    if (typeof v !== 'string') continue;
    if (v.length > MAX_VAL_LEN) continue;
    safe[k] = v;
    if (++count >= MAX_KEYS) break;
  }
  return safe;
}
function applyPlaceholders(tpl, placeholders) {
  const keys = Object.keys(placeholders).sort((a, b) => b.length - a.length);
  let html = tpl;
  for (const key of keys) {
    const val = placeholders[key];
    html = html.split(key).join(val);
  }
  return html;
}

const POOL_SIZE = Number(process.env.POOL_SIZE || Math.max(4, os.cpus()?.length || 2));
let browserPromise = null;
const pagesPool = [];
const waiters = [];
async function initBrowserPool() {
  if (!browserPromise) {
    const launchOpts = {
      headless: 'new',
      args: [
        '--no-sandbox',
        '--disable-setuid-sandbox',
        '--disable-dev-shm-usage',
        '--disable-extensions',
        '--disable-gpu',
      ],
    };
    if (process.env.PUPPETEER_EXECUTABLE_PATH) {
      launchOpts.executablePath = process.env.PUPPETEER_EXECUTABLE_PATH;
      console.log(`INFO puppeteer executablePath=${launchOpts.executablePath}`);
    }
    browserPromise = puppeteer.launch(launchOpts);
  }
  const browser = await browserPromise;

  while (pagesPool.length < POOL_SIZE) {
    const page = await browser.newPage();

    await page.setJavaScriptEnabled(false);
    await page.setRequestInterception(true);
    page.on('request', reqP => {
      const url = reqP.url();

      console.log(`REQ ${reqP.method()} ${url}`);
      try {
        const u = new URL(url);
        if (u.protocol === 'file:' || u.protocol === 'filesystem:') {
          console.log(`ABORT ${url}`);
          return reqP.abort();
        }
      } catch (e) {
        console.log(`WARN malformed URL: ${url}`);
      }
      return reqP.continue();
    });
    page.on('requestfailed', r => {
      const et = r.failure() && r.failure().errorText;
      console.log(`FAIL ${r.method()} ${r.url()} err=${et || 'unknown'}`);
    });

    pagesPool.push(page);
  }

  try {
    const p = pagesPool[0];
    await p.setContent('<html><body style="font-family: system-ui">warmup</body></html>', { waitUntil: 'load' });
    await p.pdf({ format: 'A4', printBackground: true, scale: 1 });
  } catch (_) { }
  console.log(`INFO browser pool ready size=${POOL_SIZE}`);
  return browser;
}
async function acquirePage() {
  if (pagesPool.length > 0) return pagesPool.pop();
  return new Promise(resolve => waiters.push(resolve));
}
function releasePage(page) {
  if (waiters.length > 0) {
    const w = waiters.shift();
    w(page);
  } else {
    pagesPool.push(page);
  }
}
async function shutdown() {
  try {
    const browser = await browserPromise;
    if (browser) await browser.close();
  } catch (_) { }
}

app.get('/health', (_req, res) => res.json({ ok: true, poolFree: pagesPool.length, poolSize: POOL_SIZE }));
app.post('/render', async (req, res) => {
  let placeholders;
  try {
    placeholders = parsePlaceholders(req.body || {});
  } catch (e) {
    return res.status(400).json({ ok: false, error: e.message });
  }
  const agreementId = (placeholders.agreement_id || 'unknown');
  console.log(`INFO render.start id=${short(agreementId)} keys=${Object.keys(placeholders).length}`);
  const html = applyPlaceholders(AGREEMENT_TEMPLATE, placeholders);
  console.log(html)
  let page;
  try {
    await initBrowserPool();
    page = await acquirePage();

    await page.setContent(html, { waitUntil: 'load' });

    await new Promise(r => setTimeout(r, 40));
    const pdf = await page.pdf({
      format: 'A4',
      printBackground: true,
      scale: 1,
    });
    console.log(`INFO render.ok id=${short(agreementId)} bytes=${pdf.length}`);
    res.type('application/pdf').send(pdf);
  } catch (err) {
    console.error(`ERROR render.fail id=${short(agreementId)} err=${err && err.message}`);
    res.status(500).json({ ok: false, error: 'render error' });
  } finally {
    if (page) releasePage(page);
  }
});

const port = process.env.PORT || 3000;
app.listen(port, async () => {
  await initBrowserPool().catch(e => {
    console.error(`FATAL browser init failed: ${e && e.message}`);
    process.exit(1);
  });
  console.log(`${new Date().toISOString()} pdf_service listening on ${port}`);
});
process.on('SIGTERM', async () => { await shutdown(); process.exit(0); });
process.on('SIGINT', async () => { await shutdown(); process.exit(0); });
