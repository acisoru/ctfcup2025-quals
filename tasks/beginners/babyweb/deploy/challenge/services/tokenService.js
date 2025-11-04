const jwt = require('jsonwebtoken')
const crypto = require('crypto')
const SECRET_KEY = crypto.randomBytes(64).toString('hex')

module.exports = {
    generateToken(payload) {
        payload = Object.assign(payload);
        return jwt.sign(payload, SECRET_KEY, { algorithm:'HS256' })
    },
    validateToken(token) {
        return jwt.verify(token, SECRET_KEY, { algorithm:'HS256' })
    },
    formatCredentialsHint({email, password_hash}) {
        return `Invalid credentials!<br>Try these test credentials:<br><b>Email: ${email}</b><br><b>Password: ${password_hash}</b>`
    }
}

