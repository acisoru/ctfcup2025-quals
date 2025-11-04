const axios = require('axios')
const ExchangeRateService = require('./exchangeRateService')

const CancelToken = axios.CancelToken

function isValidIPAddress(ip) {
    return /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(ip);
}

function sendApiRequest(url) {
    const source = CancelToken.source()
    setTimeout(source.cancel, 5000, 'Request timeout')
    return axios({
        method: 'GET',
        url,
        cancelToken: source.token,
        timeout: 3000
    })
}

module.exports = (clientIp) => {
    return new Promise(async resolve => {
        const validIp = isValidIPAddress(clientIp)
        if (validIp && clientIp !== '127.0.0.1') {
            try {
                const apiResponse = await sendApiRequest(`https://ipapi.co/${clientIp}/currency/`)
                if (apiResponse.status === 200 && apiResponse.data !== 'Undefined') {
                    const exchangeService = new ExchangeRateService()['rates']
                    apiResponse.data in exchangeService
                        ? resolve({currency: apiResponse.data, rate: exchangeService[apiResponse.data]})
                        : resolve({currency: 'RUB', rate: 1})
                }
            } catch (e) {
                console.log('[!] Geolocation API error:', e.message)
            }
        }
        resolve({currency: 'RUB', rate: 1})
    })
}

