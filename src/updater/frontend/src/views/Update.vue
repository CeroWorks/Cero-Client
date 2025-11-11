<template>
  <div>
    <h1>Mise à jour en cours...</h1>
    <div id="progress-container">
      <div id="progress" :style="{ width: progress + '%' }"></div>
    </div>
    <p>{{ status }}</p>
    <button @click="startUpdate">Lancer la mise à jour</button>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { StartUpdate } from '../../wailsjs/go/main/Updater.js'
import { EventsOn } from '../../wailsjs/runtime/runtime.js'

const status = ref('En attente...')
const progress = ref(0)

EventsOn('progress', (percent) => {
  progress.value = percent
  status.value = `Téléchargement... ${percent}%`
})

async function startUpdate() {
  status.value = "Téléchargement et installation en cours..."
  const result = await StartUpdate()
  status.value = result
  await Finish()
}
</script>

<style scoped>
#progress-container {
  width: 100%;
  height: 20px;
  background: #eee;
  margin: 10px 0;
  border-radius: 5px;
}
#progress {
  height: 100%;
  background: #4caf50;
  width: 0%;
  border-radius: 5px;
  transition: width 0.3s;
}
</style>
