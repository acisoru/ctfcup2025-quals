module.exports = (currencyCode, exchangeRate) => {
    const productCatalog = [
        {
            'id': 1,
            'name': 'Premium CTF Membership (Annual)',
            'category': 'Subscription',
            'description': '<li>Access to 500+ exclusive CTF challenges</li><li>Premium writeups and video solutions</li><li>Private Discord community</li><li>Monthly live training sessions</li><li>Certificate of completion</li>',
            'image': 'assets/images/web_course.jpg',
            'price': (699.99 / exchangeRate).toFixed(2),
            'currency': currencyCode,
        }
    ]

    const productsJson = JSON.stringify(productCatalog)
    const totalPrice = productCatalog.reduce((sum, product) => sum + parseFloat(product['price']), 0)
    return {products: productsJson, total: totalPrice}
}

