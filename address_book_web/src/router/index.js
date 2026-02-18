import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  { path: '/', name:'ContactList' ,component: () => import('../views/ContactList.vue') },
  { path: '/edit/:id', name: 'EditContact', component: () => import('../views/EditContact.vue') },
  { path: '/add', name: 'AddContact', component: () => import('../views/AddContact.vue') }
]

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes,
})

export default router
