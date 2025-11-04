const axios = require('axios')
const convert = require('xml-js')
const EXCHANGE_API_URL = 'https://www.cbr.ru/scripts/XML_daily.asp'

function fetchExchangeData(url) {
    return axios({
        method: 'GET',
        url,
        timeout: 3000
    })
}

class ExchangeRateService {
    constructor(ratesMap = {}) {
        if (ExchangeRateService._instance) {
            return ExchangeRateService._instance
        }
        ExchangeRateService._instance = this
        this.rates = ratesMap
    }
    
    static async initialize() {
        const xmlResponse = await fetchExchangeData(EXCHANGE_API_URL)
        const parsedRates = convert.xml2js(xmlResponse.data, {compact: true, spaces: 4})
        const ratesMap = parsedRates["ValCurs"]["Valute"]
            .reduce((accumulator, currency) =>
                Object.assign(accumulator, {
                    [currency['CharCode']._text]: parseInt(currency.Value._text, 10) / currency['Nominal']._text
                }), {})
        return new ExchangeRateService(ratesMap)
    }
}

module.exports = ExchangeRateService

