import { createRouter, createWebHashHistory } from 'vue-router'
import Home from '../views/Home.vue'
import Update from '../views/Update.vue'

const routes = [
  { path: '/', component: Home },
  { path: '/update', component: Update },
]

const router = createRouter({
  history: createWebHashHistory(),
  routes,
})

export default router
