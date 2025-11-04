const crypto = require('crypto')

const accountsSet = new Set()
while (accountsSet.size < 10000) accountsSet.add(`ctf_${crypto.randomBytes(4).toString('hex')}@ctfmarket.com`)
const accountsData = [...accountsSet].map(email => [email, crypto.randomBytes(8).toString('hex'), 100, 0])

module.exports = () => dbManager.initializeAccounts(accountsData)

