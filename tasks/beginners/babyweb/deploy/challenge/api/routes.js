const express = require('express')
const router = express.Router()
const { exec } = require("child_process")
const { Validator } = require("express-json-validator-middleware")
const { validate } = new Validator()
const { ValidationError } = require("express-json-validator-middleware")
const tokenService = require('../services/tokenService')
const geolocationService = require('../services/geolocationService')
const productGenerator = require('../utils/productGenerator')
const authMiddleware = require('./authMiddleware')
const authSchema = require('../schemas/authSchema')
const path = require("path")
const rateLimit = require("express-rate-limit")

const apiRateLimiter = rateLimit({
    windowMs: 10 * 1000, // 10 seconds
    max: 5,
    standardHeaders: true,
    legacyHeaders: false,
})

const sendResponse = data => ({ response: data })
const pagesPath = path.join(__dirname, '/../pages/')
const logsPath = path.join(__dirname, '/../logs/')

router.get('/', (req, res) => res.sendFile(pagesPath + 'home.html'))
router.get('/shop', authMiddleware, (req, res) => res.sendFile(pagesPath + 'shop.html'))
router.get('/signout', (req, res) => res.clearCookie('auth_token').redirect('/'))
router.get('/debug', (req, res) => exec('tail -n 100 ' + logsPath + 'requests.log', (e, o) => res.type('txt').send(o)))

router.post('/api/auth',
    validate({ body: authSchema }),
    ({body: {email, password}, headers: {'x-forwarded-for': userIp}}, res) => {
        dbManager.authenticateUser(email, password).then(async account => {
            const {currency, rate} = await geolocationService(userIp)
            console.log('[+] Login from IP:', userIp, '| Currency:', currency, '| Rate:', rate)
            const {products, total} = productGenerator(currency, rate)
            await dbManager.updateShoppingBasket(account.id, currency, products, total).catch(e => console.log('[!] Basket error:', e.message))
            let authToken = tokenService.generateToken({email: account.email, id: account.id, currency, rate})
            res.cookie('auth_token', authToken, { maxAge: 3600000 * 48, httpOnly: true })
            res.send(sendResponse('Authentication successful!'))
        }).catch(() => {
            setTimeout(() => {
                dbManager.getRandomAccount()
                    .then(account => res.status(403).send(sendResponse(tokenService.formatCredentialsHint(account))))
                    .catch(() => res.status(500).send('Internal server error'))
            }, 500)
        })
    })

router.post('/api/purchase', authMiddleware, async ({user: {id: account_id}}, res) => {
    dbManager.processPurchase(account_id)
        .then(() => res.send(sendResponse("Purchase completed successfully")))
        .catch(() => res.status(402).send(sendResponse("Insufficient CTFCoins")))
})

router.get('/api/basket', authMiddleware, async ({user: {id: account_id}}, res) => {
    dbManager.getBasketByAccountId(account_id)
        .then(basket => res.send(sendResponse(basket)))
        .catch(() => res.status(500).send('Internal server error'))
})

router.get('/api/account', authMiddleware, async ({user: {id: account_id, currency: currencyCode, rate: exchangeRate}}, res) => {
    dbManager.getAccountById(account_id)
        .then(account => {
            delete account['purchased']
            account['currency'] = 'RUB-' + currencyCode
            account['exchange_rate'] = exchangeRate
            res.send(sendResponse(account))
        })
        .catch(() => res.status(500).send(sendResponse('Internal server error')))
})

router.get('/purchase-details', authMiddleware, ({user: {id: account_id}, query: {secret}}, res) => {
    if (secret !== 'premium') return res.send('¯\\_(ツ)_/¯')
    dbManager.getAccountById(account_id)
        .then(account => {
            const flag = process.env['FLAG']
            account['purchased']
                ? res.send(`<p>Thank you for your purchase! ${account['coins']} CTFCoins remaining!</p><br><p style="color: red; font-weight: bold;">${flag}</p>`)
                : res.send('¯\\_(ツ)_/¯')
        })
        .catch(() => res.status(500).send(sendResponse('Internal server error')))
})

router.post('/api/bonus', apiRateLimiter, authMiddleware, ({user: {id: account_id}, query: {code}}, res) => {
    if (code !== 'BONUS-CTF-COIN') return res.send(sendResponse("Invalid bonus code"))
    setTimeout(async () => {
        dbManager.addBonusCoins(account_id)
            .then(() => res.send(sendResponse("Bonus applied: +0.00001 CTFCoin")))
            .catch(() => res.status(402).send(sendResponse("Error applying bonus")))
    }, 500)
})

router.use((err, req, res, next) => {
    err instanceof ValidationError
        ? res.status(403).send(sendResponse('Required parameters missing!'))
        : next(err)
})

module.exports = () => router

