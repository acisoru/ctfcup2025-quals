let mysql = require('mysql')

class DatabaseManager {
    constructor() {
        this.connection = mysql.createConnection({
            multipleStatements: true,
            host: 'mariadb',
            user: 'ctfmarket_user',
            password: 'ctfmarket_pass',
            database: 'ctfmarket_db'
        })
    }

    async getRandomAccount() {
        return new Promise(async (resolve, reject) => {
            let stmt = 'SELECT * FROM accounts WHERE coins = 100 AND purchased = 0 ORDER BY RAND() LIMIT 1'
            this.connection.query(stmt, (err, result) => err || !result.length
                ? reject(err)
                : resolve(result[0]))
        })
    }

    async initializeAccounts(values) {
        return new Promise(async (resolve, reject) => {
            const stmt = `
                TRUNCATE TABLE accounts;
                ALTER TABLE accounts AUTO_INCREMENT = 500001337;
                TRUNCATE TABLE shopping_basket; 
                INSERT INTO accounts (email, password_hash, coins, purchased) VALUES ?`
            this.connection.query(stmt, [values], (err) => err
                ? reject(err)
                : resolve())
        })
    }

    async updateShoppingBasket(account_id, currency_code, products, total_price) {
        return new Promise(async (resolve, reject) => {
            const stmt = 'REPLACE INTO shopping_basket (account_id, currency_code, products, total_price, payment_status) VALUES (?, ?, ?, ?, 0)'
            this.connection.query(stmt, [account_id, currency_code, products, total_price], (err, result) => err
                ? reject(err)
                : resolve(result))
        })
    }

    async processPurchase(account_id) {
        return new Promise(async (resolve, reject) => {
            let stmt = 'UPDATE accounts SET coins=coins-(SELECT total_price FROM shopping_basket WHERE account_id=?),purchased=1 WHERE id=?'
            this.connection.query(stmt, [account_id, account_id], (err, result) => err
                ? reject(err)
                : resolve(result))
        })
    }

    async getBasketByAccountId(account_id) {
        return new Promise(async (resolve, reject) => {
            let stmt = 'SELECT * FROM shopping_basket WHERE account_id = ?'
            this.connection.query(stmt, [account_id], (err, result) => err || !result.length
                ? reject(err)
                : resolve(result[0]))
        })
    }

    async getAccountById(account_id) {
        return new Promise(async (resolve, reject) => {
            let stmt = 'SELECT id,email,coins,purchased FROM accounts WHERE id = ?';
            this.connection.query(stmt, [account_id], (err, result) => err || !result.length
                ? reject(err)
                : resolve(result[0]))
        })
    }

    async addBonusCoins(account_id) {
        return new Promise(async (resolve, reject) => {
            let stmt = 'UPDATE accounts SET coins=coins+0.00001 WHERE id = ?';
            this.connection.query(stmt, [account_id], (err, result) => err
                ? reject(err)
                : resolve(result))
        })
    }

    async authenticateUser(email, password) {
        return new Promise(async (resolve, reject) => {
            let stmt = 'SELECT email, id FROM accounts WHERE email = ? AND password_hash = ?'
            this.connection.query(stmt, [email, password], (err, result) => err || !result.length
                ? reject(err)
                : resolve(result[0]))
        })
    }
}

module.exports = DatabaseManager

