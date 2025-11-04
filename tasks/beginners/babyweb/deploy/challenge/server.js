const DatabaseManager = require('./db/dbManager')
global.dbManager     = new DatabaseManager()
const express        = require('express')
const app            = express()
const path           = require('path')
const cookieParser   = require('cookie-parser')
const apiRoutes      = require('./api/routes')
const morgan         = require('morgan')
const fs             = require("fs")
const exchangeRateService = require('./services/exchangeRateService')
const initializeUsers     = require('./utils/userInitializer')
const PORT           = 7331

const requestLogStream = fs.createWriteStream(path.join(__dirname, '/logs/requests.log'), { flags: 'a' })
app.use(morgan('combined', { stream: requestLogStream }))
app.use(express.json())
app.use(cookieParser())
app.disable('etag')
app.disable('x-powered-by')

app.use('/assets', express.static(path.resolve('public')))

app.use(apiRoutes())
app.all('*', (req, res) => res.status(404).send({error: 'Endpoint not found'}))

!(() => {
    if (!process.env['FLAG']) process.env['FLAG'] = 'FLAG=ctfmarket{FAKE_TEST_FLAG}'
    console.log('[*] Flag loaded:', process.env['FLAG'])
    initializeUsers()
        .then(() => exchangeRateService.initialize())
        .then(({rates}) => app.listen(PORT, '0.0.0.0', () => console.log(`[+] CTFMarket running on port ${PORT}\n[+] Exchange rates loaded: ${JSON.stringify(rates)}`)))
        .catch(e => console.log(`[!] FATAL ERROR: exchangeRateService | ${e.message}`))
})()

