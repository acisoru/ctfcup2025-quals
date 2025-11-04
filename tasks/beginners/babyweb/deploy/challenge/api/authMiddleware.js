const tokenService = require('../services/tokenService')

module.exports = (req, res, next) => {
    if (!req.cookies.auth_token) {
        return !req.is('application/json')
            ? res.redirect('/')
            : res.status(401).json({ error: 'Authentication required'})
    }
    try {
        req.user = tokenService.validateToken(req.cookies.auth_token)
        next()
    } catch (e) {
        res.redirect('/signout')
    }
}

