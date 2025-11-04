let currentUser = null;

// Utility functions
function showMessage(message, type = 'success') {
    const messageEl = document.getElementById('message');
    messageEl.textContent = message;
    messageEl.className = `message ${type}`;
    messageEl.classList.remove('hidden');
    
    setTimeout(() => {
        messageEl.classList.add('hidden');
    }, 3000);
}

async function apiCall(url, options = {}) {
    try {
        const response = await fetch(url, {
            headers: {
                'Content-Type': 'application/json',
                ...options.headers,
            },
            ...options,
        });
        
        const data = await response.json();
        
        if (!response.ok) {
            throw new Error(data.error || 'Something went wrong');
        }
        
        return data;
    } catch (error) {
        showMessage(error.message, 'error');
        throw error;
    }
}

// Auth functions
async function login() {
    const username = document.getElementById('login-username').value;
    const password = document.getElementById('login-password').value;
    
    if (!username || !password) {
        showMessage('Please enter username and password', 'error');
        return;
    }
    
    try {
        await apiCall('/login', {
            method: 'POST',
            body: JSON.stringify({ username, password }),
        });
        
        showMessage('Login successful!');
        updateUI();
        clearAuthForms();
    } catch (error) {
        // Error is already handled in apiCall
    }
}

async function register() {
    const username = document.getElementById('register-username').value;
    const password = document.getElementById('register-password').value;
    
    if (!username || !password) {
        showMessage('Please enter username and password', 'error');
        return;
    }
    
    try {
        await apiCall('/register', {
            method: 'POST',
            body: JSON.stringify({ username, password }),
        });
        
        showMessage('Registration successful! Please login.');
        clearAuthForms();
    } catch (error) {
        // Error is already handled in apiCall
    }
}

async function logout() {
    try {
        await apiCall('/logout', { method: 'POST' });
        showMessage('Logged out successfully');
        updateUI();
    } catch (error) {
        // Error is already handled in apiCall
    }
}

// String functions
async function createString() {
    const name = document.getElementById('string-name').value;
    const content = document.getElementById('string-content').value;
    
    if (!name || !content) {
        showMessage('Please enter both name and content', 'error');
        return;
    }
    
    try {
        await apiCall('/strings', {
            method: 'POST',
            body: JSON.stringify({ name, content }),
        });
        
        showMessage('String created successfully!');
        document.getElementById('string-name').value = '';
        document.getElementById('string-content').value = '';
        getAllStrings(); // Refresh the list
    } catch (error) {
        // Error is already handled in apiCall
    }
}

async function getString() {
    const name = document.getElementById('search-name').value;
    
    if (!name) {
        showMessage('Please enter a string name', 'error');
        return;
    }
    
    try {
        const data = await apiCall(`/strings/${name}`);
        const resultEl = document.getElementById('search-result');
        resultEl.innerHTML = `
            <div class="string-item">
                <strong>Name:</strong> ${data.name}<br>
                <strong>Content:</strong> ${data.content}
            </div>
        `;
    } catch (error) {
        document.getElementById('search-result').innerHTML = '';
    }
}

async function getAllStrings() {
    try {
        const data = await apiCall('/strings');
        const resultEl = document.getElementById('strings-result');
        
        if (data.strings.length === 0) {
            resultEl.innerHTML = '<p>No strings found. Create your first string!</p>';
            return;
        }
        
        resultEl.innerHTML = data.strings.map(string => `
            <div class="string-item">
                <strong>Name:</strong> ${string.name}<br>
                <strong>Content:</strong> ${string.content}
            </div>
        `).join('');
    } catch (error) {
        document.getElementById('strings-result').innerHTML = '';
    }
}

// UI management
function updateUI() {
    const loginForm = document.getElementById('login-form');
    const registerForm = document.getElementById('register-form');
    const userInfo = document.getElementById('user-info');
    const appSection = document.getElementById('app-section');
    
    // Check if user is logged in by trying to get profile
    fetch('/profile')
        .then(response => {
            if (response.ok) {
                return response.json();
            }
            throw new Error('Not logged in');
        })
        .then(data => {
            currentUser = data;
            loginForm.classList.add('hidden');
            registerForm.classList.add('hidden');
            userInfo.classList.remove('hidden');
            appSection.classList.remove('hidden');
            document.getElementById('username-display').textContent = data.username;
        })
        .catch(() => {
            currentUser = null;
            loginForm.classList.remove('hidden');
            registerForm.classList.remove('hidden');
            userInfo.classList.add('hidden');
            appSection.classList.add('hidden');
        });
}

function clearAuthForms() {
    document.getElementById('login-username').value = '';
    document.getElementById('login-password').value = '';
    document.getElementById('register-username').value = '';
    document.getElementById('register-password').value = '';
}

// Event listeners for Enter key
document.addEventListener('DOMContentLoaded', function() {
    // Login form Enter key
    document.getElementById('login-password').addEventListener('keypress', function(e) {
        if (e.key === 'Enter') {
            login();
        }
    });
    
    // Register form Enter key
    document.getElementById('register-password').addEventListener('keypress', function(e) {
        if (e.key === 'Enter') {
            register();
        }
    });
    
    // String creation Enter key
    document.getElementById('string-content').addEventListener('keypress', function(e) {
        if (e.key === 'Enter' && e.ctrlKey) {
            createString();
        }
    });
    
    // Search Enter key
    document.getElementById('search-name').addEventListener('keypress', function(e) {
        if (e.key === 'Enter') {
            getString();
        }
    });
    
    // Initial UI update
    updateUI();
});
