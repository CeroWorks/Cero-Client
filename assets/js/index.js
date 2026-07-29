const canvas = document.getElementById('grainCanvas');
const ctx = canvas.getContext('2d');
let width, height;

function resize() {
    width  = canvas.width  = window.innerWidth;
    height = canvas.height = window.innerHeight;
}
window.addEventListener('resize', resize);
resize();

function animateGrain() {
    const imageData = ctx.createImageData(width, height);
    const data = imageData.data;
    for (let i = 0; i < data.length; i += 4) {
        const val = Math.random() * 255;
        data[i] = data[i+1] = data[i+2] = val;
        data[i+3] = 255;
    }
    ctx.putImageData(imageData, 0, 0);
    requestAnimationFrame(animateGrain);
}
animateGrain();

document.body.addEventListener('mousedown', (e) => {
    if (e.button !== 0) return;
    if (e.target.closest('a, button, input, select, textarea')) return;
    if (window.drag_start) window.drag_start();
});

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