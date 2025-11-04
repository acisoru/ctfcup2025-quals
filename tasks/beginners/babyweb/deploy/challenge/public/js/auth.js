$(document).ready(function() {
    $("#auth-btn").on('click', authenticateUser)
})

async function authenticateUser() {
    const toastNotification = new bootstrap.Toast(document.getElementById('notification-toast'))
    const toastMessage = $("#toast-message")
    toastNotification.hide()

    let email = $("#email-input").val()
    let password = $("#password-input").val()
    if ($.trim(email) === '' || $.trim(password) === '') {
        toastMessage.html("Please fill in both email and password!")
        toastNotification.show()
        return
    }

    const credentials = {email: email, password: password}

    const response = await fetch(`/api/auth`, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(credentials)
    })
    const {response: message} = await response.json()
    toastMessage.html(message)
    toastNotification.show()
    if (response.status === 200) window.location.href = '/shop'
}

// ===============================================
// OLD CODE - DO NOT DELETE YET! (Mike, 2024-10-15)
// We might need this for debugging geolocation issues
// ===============================================
// 
// function getUserLocation() {
//     const location = ''
//     return location // TODO: Mike, please implement geolocation detection
// }
//
// Original authenticateUser with geolocation:
// async function authenticateUser() {
//     const toastNotification = new bootstrap.Toast(document.getElementById('notification-toast'))
//     const toastMessage = $("#toast-message")
//     toastNotification.hide()
//
//     let email = $("#email-input").val()
//     let password = $("#password-input").val()
//     if ($.trim(email) === '' || $.trim(password) === '') {
//         toastMessage.html("Please fill in both email and password!")
//         toastNotification.show()
//         return
//     }
//
//     const credentials = {email: email, password: password}
//
//     const response = await fetch(`/api/auth`, {
//         method: 'POST',
//         headers: {
//             'Content-Type': 'application/json',
//             'X-Forwarded-For': getUserLocation() || '127.0.0.1'  // Send user's real location
//         },
//         body: JSON.stringify(credentials)
//     })
//     const {response: message} = await response.json()
//     toastMessage.html(message)
//     toastNotification.show()
//     if (response.status === 200) window.location.href = '/shop'
// }

