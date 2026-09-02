(function(window) {
    "use strict";
    const Cero = window.Cero = window.Cero || {};
const STEPS = [
    { key: 'account',   label: 'Compte' },
    { key: 'manifest',  label: 'Manifest' },
    { key: 'client',    label: 'Client JAR' },
    { key: 'libraries', label: 'Librairies' },
    { key: 'natives',   label: 'Natives' },
    { key: 'assets',    label: 'Assets' },
    { key: 'java',      label: 'Java' },
    { key: 'launch',    label: 'Lancement' },
];

function buildStepsList() {
    document.getElementById('stepsList').innerHTML = STEPS.map(s =>
        `<div class="step-item pending" id="step-${s.key}"><div class="step-dot"></div><span class="text-xs font-medium">${s.label}</span></div>`
    ).join('');
}

function overlayShow(version) {
    buildStepsList();
    document.getElementById('overlayVersion').textContent = 'Minecraft ' + version;
    setOverlayProgress('Initialisation...', 0);
    document.getElementById('launchOverlay').classList.add('active');
}

function overlayHide() { document.getElementById('launchOverlay').classList.remove('active'); }

function setOverlayProgress(step, pct) {
    document.getElementById('currentStepLabel').textContent = step;
    document.getElementById('progressBar').style.width = pct + '%';
    document.getElementById('progressPct').textContent  = pct + '%';
}

function overlayError(msg) {
    document.getElementById('currentStepLabel').textContent = 'Error : ' + msg;
    document.getElementById('progressBar').style.background = '#f87171';
    setTimeout(() => overlayHide(), 3000);
}

window.onLaunchProgress = function(step, pct) {
    setOverlayProgress(step, pct);
    if (pct >= 100) {
        setTimeout(() => {
            overlayHide();
            const btn = document.getElementById('launchBtn');
            if(btn) btn.disabled = false;
            updatePlayButton();
        }, 800);
    }
};

window.onLaunchError = function(msg) {
    overlayError(msg);
    const btn = document.getElementById('launchBtn');
    if(btn) btn.disabled = false;
    updatePlayButton();
};

window._onGameStart = function() {
    document.getElementById('playGroup').style.display = 'none';
    document.getElementById('killBtn').style.display   = 'block';

    if (window.get_settings) {
        window.get_settings().then(function(s) {
            if (s && s.hideOnLaunch && window.close_to_tray) {
                window.close_to_tray();
            }
        }).catch(() => {});
    }
};

window._onGameStop = function() {
    document.getElementById('playGroup').style.display = 'flex';
    document.getElementById('killBtn').style.display   = 'none';

    if (window.get_settings) {
        window.get_settings().then(function(s) {
            if (s && s.reopenOnStop && window.show_window) {
                window.show_window();
            }
        }).catch(() => {});
    }
};

async function launchGame() {
    const selectedVersion = Cero.state && Cero.state.selectedVersion;
    if (!selectedVersion) { document.getElementById('launchStatus').textContent = 'Pas de version'; return; }
    const btn = document.getElementById('launchBtn');
    btn.disabled = true;
    btn.innerHTML = 'Lancement...';
    overlayShow(Cero.state && Cero.state.selectedVersion ? Cero.state.selectedVersion.id : null);
    try {
        await window.launch_mc(selectedVersion.id);
    }   
    catch(e) { overlayError(e?.message || String(e)); }
}

async function killGame() { if (window.kill_game) await window.kill_game(); }

    Cero.modules = Cero.modules || {};

    Cero.modules.buildStepsList = buildStepsList;
    Cero.modules.overlayShow = overlayShow;
    Cero.modules.overlayHide = overlayHide;
    Cero.modules.setOverlayProgress = setOverlayProgress;
    Cero.modules.overlayError = overlayError;
    Cero.modules.launchGame = launchGame;
    Cero.modules.killGame = killGame;

    window.buildStepsList = buildStepsList;
    window.overlayShow = overlayShow;
    window.overlayHide = overlayHide;
    window.setOverlayProgress = setOverlayProgress;
    window.overlayError = overlayError;
    window.launchGame = launchGame;
    window.killGame = killGame;

})(window);
