$(document).ready(() => {
    getAccountInfo()
    getBasketContents()
    $("#apply-bonus-btn").on('click', applyBonus)
    $("#purchase-btn").on('click', completePurchase)
    $("#bonus-code-input").val("BONUS-CTF-COIN")
})

const toastNotification = new bootstrap.Toast(document.getElementById('notification-toast'))
const toastMessage = $("#toast-message")
toastNotification.hide()

function getAccountInfo() {
    fetch("/api/account", {"credentials": "include"})
        .then(r => r.json()).then(data => {
            $("#account-email").text(data.response['email'])
            $("#account-id").text(data.response['id'])
            $("#account-balance").text(data.response['coins'])
        })
}

const productTemplate = (thumbnail, name, category, price, currency, details) => `
<tr>
    <td>
        <div class="card border-0">
            <img src="${thumbnail}" class="product-thumbnail" alt="product">
            <div class="card-body p-2">
                <h6 class="card-title mb-0">${name}</h6>
                <small class="text-muted">${category}</small>
            </div>
        </div>
    </td>
    <td><small>${details}</small></td>
    <td>1</td>
    <td><strong>${price} ${currency}</strong></td>
</tr>
`

function getBasketContents() {
    fetch("/api/basket", {"credentials": "include"})
        .then(r => r.json()).then(data => {
            const products = JSON.parse(data.response['products'])
            $("#products-container").html(products.map(p => productTemplate(p.image, p.name, p.category, p.price, p.currency, p.description)).join(''))
            $("#total-amount").text(data.response.total_price + ' ' + data.response.currency_code)
        })
}

async function applyBonus() {
    let code = $("#bonus-code-input").val()
    fetch("/api/bonus?code=" + code, {"credentials": "include", "method": "POST"})
        .then(r => r.json()).then(data => {
            toastMessage.html(data.response)
            toastNotification.show()
        }).then(() => {
            getAccountInfo()
        })
}

async function completePurchase() {
    const response = await fetch("/api/purchase", {"credentials": "include", "method": "POST"})
    const {response: message} = await response.json()
    toastMessage.html(message)
    toastNotification.show()
    if (response.status === 200) {
        // Redirect to order confirmation
        window.location.href = '/shop?success=true'
    }
}

// ===============================================
// LEGACY CODE - Keep for reference (Sarah, 2024-10-20)
// Old purchase flow before we simplified it
// ===============================================
//
// Original completePurchase function:
// async function completePurchase() {
//     const response = await fetch("/api/purchase", {"credentials": "include", "method": "POST"})
//     const {response: message} = await response.json()
//     toastMessage.html(message)
//     toastNotification.show()
//     if (response.status === 200) {
//         // Old redirect to premium details page
//         window.location.href = '/purchase-details?secret=premium'
//     }
// }
//
// Also had this bonus code feature:
// $("#bonus-code-input").val("BONUS-CTF-COIN")  // Auto-fill the working code

