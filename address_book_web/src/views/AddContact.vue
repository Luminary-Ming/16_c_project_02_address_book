<template>
    <div class="mobile edit-page">
        <div class="title-row">
            <button class="text-btn" @click="$router.back()">取消</button>
            <div class="title">新建联系人</div>
            <button class="text-btn" @click="saveContact">完成</button>
        </div>

        <div class="avatar-area">
            <div class="avatar-container" @click="triggerFileInput">
                <div class="avatar-circle">
                    <span v-if="!contact.image" class="avatar-placeholder">+</span>
                    <img v-else :src="contact.image" alt="头像" class="avatar-img" />
                </div>
            </div>
            <input type="file" ref="fileInput" style="display: none" accept="image/jpeg,image/png,image/webp" @change="handleFileSelect" />
        </div>

        <div class="form-card">
            <label class="field"><span class="label">姓名</span>
                <input v-model="contact.name" placeholder="姓名" />
            </label>
            <label class="field"><span class="label">电话</span>
                <input v-model="contact.telephone" placeholder="电话" />
            </label>
            <label class="field"><span class="label">邮箱</span>
                <input v-model="contact.email" placeholder="邮箱（可选）" />
            </label>
        </div>
        <div v-if="showToast" class="toast">{{ toastMsg }}</div>
    </div>
</template>

<script setup>
import { reactive, ref } from 'vue';
import { useRouter } from 'vue-router';
import { contactApi } from '@/api/contact';
import { ElMessage } from 'element-plus';
import request from "@/utils/request";

const router = useRouter();
const contact = reactive({ name: '', telephone: '', email: '', image: '' });
const showToast = ref(false);
const toastMsg = ref('');
const fileInput = ref(null);

function triggerFileInput() {
    fileInput.value.click();
}

async function saveContact() {
    const payload = {
        name: contact.name,
        telephone: contact.telephone,
        email: contact.email || '',
        image: contact.image,
    };

    let created = null;
    const raw = localStorage.getItem('contacts');
    const list = raw ? JSON.parse(raw) : [];

    const res = await contactApi.addContact(payload);

    if (res && (res.data?.id || res.id || (res.data?.data && res.data.data.id))) {
        created = res.data?.id ? res.data : res.id ? res : res.data.data;
    }

    if (created && created.id) {
        list.push({
            id: created.id,
            name: created.name || payload.name,
            telephone: created.telephone || payload.telephone,
            email: created.email || payload.email,
            image: created.image || payload.image,
        });
    } else {
        const maxId = list.reduce((m, c) => Math.max(m, c.id || 0), 0);
        const id = maxId + 1;
        list.push({ id, name: payload.name, telephone: payload.telephone, email: payload.email, image: payload.image });
    }

    localStorage.setItem('contacts', JSON.stringify(list));
    ElMessage.success('添加成功!');
    showDone('已保存');
}

async function handleFileSelect(event) {
    const file = event.target.files[0];
    if (!file) return;

    // 检查文件类型
    const allowedTypes = ['image/jpeg', 'image/png', 'image/webp'];
    if (!allowedTypes.includes(file.type)) {
        ElMessage.error('只支持 JPG, PNG, WebP 格式');
        return;
    }

    // 检查文件大小（限制10MB）
    if (file.size > 10 * 1024 * 1024) {
        ElMessage.error('图片大小不能超过10MB');
        return;
    }

    try {
        // 转换为Base64
        const base64 = await fileToBase64(file);

        // 上传到服务器
        const response = await uploadBase64Image({
            filename: file.name,
            data: base64
        });

        let imageUrl;
        if (response && response.data && response.data.data.url) {
            imageUrl = response.data.data.url;
            contact.image = imageUrl;
            ElMessage.success('头像上传成功');
        } else {
            ElMessage.error('上传失败');
        }
    } catch (error) {
        console.error('上传失败:', error);
        ElMessage.error('上传失败: ' + error.message);
    }

    // 重置input
    event.target.value = '';
}

// 文件转Base64
function fileToBase64(file) {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = () => {
            // 去掉Base64前缀（如 "data:image/jpeg;base64,"）
            const base64String = reader.result.split(',')[1];
            resolve(base64String);
        };
        reader.onerror = reject;
        reader.readAsDataURL(file);
    });
}

// 上传Base64图片
async function uploadBase64Image(imageData) {
    return await request({
        url: '/contacts/upload',
        method: 'post',
        data: imageData,
        headers: {
            'Content-Type': 'application/json'
        }
    });
}

function showDone(msg) {
    toastMsg.value = msg;
    showToast.value = true;

    setTimeout(() => {
        showToast.value = false;
        router.push({ name: 'ContactList' });
    }, 500);
}
</script>

<style scoped>
.mobile {
    max-width: 480px;
    height: 900px;
    margin: 20px auto;
    background: #0f0f10;
    color: #fff;
    padding-bottom: 40px;
    border-radius: 6px;
}

.title-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 14px 16px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.03)
}

.title {
    font-size: 20px;
    font-weight: 700
}

.text-btn {
    background: transparent;
    color: #22c55e;
    border: none;
    font-size: 18px;
    cursor: pointer
}

.avatar-area {
    display: flex;
    justify-content: center;
    padding: 20px 0;
    position: relative;
}

.avatar-container {
    position: relative;
    width: 88px;
    height: 88px;
    cursor: pointer;
}

.avatar-circle {
    width: 88px;
    height: 88px;
    border-radius: 50%;
    background: #222;
    display: flex;
    align-items: center;
    justify-content: center;
    overflow: hidden;
    /* 关键：确保图片不超出圆形 */
    border: 2px solid rgba(34, 197, 94, 0.3);
    /* 添加边框 */
}

.avatar-placeholder {
    color: #22c55e;
    font-size: 48px;
    font-weight: 300;
    line-height: 1;
}

.avatar-img {
    width: 100%;
    height: 100%;
    object-fit: cover;
    /* 关键：保持图片比例并填充容器 */
    object-position: center;
    /* 图片居中显示 */
}

/* 添加悬停效果 */
.avatar-container:hover .avatar-circle {
    border-color: #22c55e;
    transform: scale(1.05);
    transition: all 0.3s ease;
}


.avatar-large {
    width: 88px;
    height: 88px;
    border-radius: 50%;
    background: #222;
    display: flex;
    align-items: center;
    justify-content: center;
    color: #22c55e;
    font-size: 48px
}

.form-card {
    padding: 12px 16px;
}

.field {
    display: block;
    background: #121212;
    margin-bottom: 12px;
    border-radius: 12px;
    padding: 12px;
}

.label {
    display: block;
    color: #9aa0a6;
    font-size: 18px;
    margin-bottom: 6px
}

input {
    width: 100%;
    background: transparent;
    border: 0;
    color: #fff;
    font-size: 16px;
    outline: none;
    padding: 16px 0
}

.toast {
    position: absolute;
    left: 50%;
    transform: translateX(-50%);
    bottom: 250px;
    background: rgba(0, 0, 0, 0.7);
    color: #fff;
    padding: 18px 24px;
    border-radius: 18px;
    font-size: 15px
}
</style>