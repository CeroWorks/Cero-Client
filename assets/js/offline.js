const statusText = document.getElementById('statusText');

async function checkAndRedirect() {
    try {
        if (statusText) statusText.textContent = 'Vérification en cours';
        const hasInternet = await window.checkInternet();
        
        if (hasInternet) {
            if (statusText) statusText.textContent = 'Connexion détectée !';
            await new Promise(r => setTimeout(r, 600));
            window.location.href = 'index.html';
        }
    } catch (_) { }
}

checkAndRedirect();

setInterval(checkAndRedirect, 3000);