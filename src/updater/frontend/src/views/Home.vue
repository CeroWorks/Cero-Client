<template>
  <div>
    <h1>Updater</h1>
    <p>{{ status }}</p>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { FetchLatestVersion, FileExists, ReadVersionFile, Finish } from '../../wailsjs/go/main/Updater.js'

const status = ref('Vérification en cours...')
const router = useRouter()

async function checkUpdate() {
  try {
    const latestVersion = await FetchLatestVersion()
    let localVersion = null

    if (await FileExists()) {
      localVersion = await ReadVersionFile()
    }

    if (!localVersion || localVersion.trim() < latestVersion) {
      status.value = `Nouvelle version disponible (${latestVersion}) !`
      router.push('/update')
    } else {
      status.value = `Votre version est à jour (${localVersion})`
      await Finish()
    }
  } catch (err) {
    status.value = `Erreur: ${err.message}`
  }
}

onMounted(() => checkUpdate())
</script>
