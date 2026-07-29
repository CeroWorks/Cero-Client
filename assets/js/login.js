const canvas = document.getElementById('grainCanvas');
const ctx = canvas.getContext('2d');
let width, height;

const playerName = document.getElementById('playerName');
if (playerName) playerName.remove();

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

async function loginWithMicrosoft() {
    const btnSpan = document.querySelector('.ms-btn span');
    if (!btnSpan) return;

    const originalText = btnSpan.innerText;
    btnSpan.innerText = "Connexion en cours...";

    try {
        if (!window.loginMicrosoft) throw new Error("loginMicrosoft non disponible");
        await window.loginMicrosoft();
        btnSpan.innerText = "Connecté ✓";
        setTimeout(() => { window.location.href = "app.html"; }, 500);
    } catch (e) {
        console.error("[Login] Microsoft Auth Error:", e);
        btnSpan.innerText = "Échec — réessayez";
        setTimeout(() => { btnSpan.innerText = originalText; }, 2500);
    }
}

document.querySelector('.ms-btn')?.addEventListener('click', loginWithMicrosoft);
window.loginWithMicrosoft = loginWithMicrosoft;

document.addEventListener('mousedown', (e) => {
    if (e.button !== 0) return;
    const NO_DRAG = 'a,button,input,select,textarea,canvas,.ms-btn,.wc-btn,[onclick],.nodrag';
    if (e.target.closest(NO_DRAG)) return;
    if (!e.target.closest('.app-drag')) return;
    
    if (window.drag_start) {
        e.preventDefault();
        window.drag_start();
    }
}, true);
