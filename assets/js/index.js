const statusText = document.getElementById('statusText');
window.setLoadingStatus = (txt) => { statusText.textContent = txt; };

async function boot() {
    try {
        window.setLoadingStatus("Vérification de la connexion");
        await new Promise(r => setTimeout(r, 600));
        const hasInternet = await window.checkInternet();

        if (!hasInternet) {
            window.setLoadingStatus("Mode hors-ligne");
            await new Promise(r => setTimeout(r, 500));
            window.location.href = "offline.html";
            return;
        }

        window.setLoadingStatus("Vérification du compte...");
        const hasAccount = await window.checkAccount();

        if (!hasAccount) {
            window.setLoadingStatus("Connexion requise");
            await new Promise(r => setTimeout(r, 500));
            window.location.href = "login.html";
            return;
        }

        window.setLoadingStatus("Chargement de l'interface");
        await new Promise(r => setTimeout(r, 600));
        window.location.href = "app.html";

    } catch (e) {
        window.setLoadingStatus("Erreur : " + e.message);
        console.error(e);
    }
}

boot();