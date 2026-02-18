import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'

export default defineConfig({
    plugins: [
        vue(),
        vueDevTools(),
    ],
    resolve: {
        alias: {
        '@': fileURLToPath(new URL('./src', import.meta.url))
        },
    },
    server: {
        port: 5173,
        //open: true,  // 自动打开浏览器
        host: '0.0.0.0',  // 监听所有网络接口，允许外部访问
        cors: true,  // 启用跨域资源共享
        allowedHosts: [
            'addressbook.f1.luyouxia.net',  // 穿透域名
        ],
        proxy: {
            '/api': {
                target: 'http://192.168.2.227:8080',  // 后端地址
                changeOrigin: true,  // 解决跨域
              
            },
        }
    },
})
