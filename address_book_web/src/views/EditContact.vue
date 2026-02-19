<template>
    <div class="mobile edit-page">
        <div class="title-row">
            <button class="text-btn" @click="$router.back()">取消</button>
            <div class="title">编辑联系人</div>
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
import { reactive, ref, onMounted } from 'vue';
import { useRouter, useRoute } from 'vue-router';
import { contactApi } from '@/api/contact';
import { ElMessage } from 'element-plus';
import request from "@/utils/request";

const router = useRouter();
const route = useRoute();
const contact = reactive({ id: null, name: '', telephone: '', email: '', image: '' });
const showToast = ref(false);
const toastMsg = ref('');


const fileInput = ref(null);

function triggerFileInput() {
    fileInput.value.click();
}

async function loadContact() {
    const contactId = Number(route.params.id);
    const res = await contactApi.getContact(contactId);
    let found = null;
    if (res) {
        if (res.data && res.data.id) found = res.data;
        else if (res.id) found = res;
        else if (res.data && res.data.data) found = res.data.data;
    }
    if (found) {
        contact.id = found.id;
        contact.name = found.name || '';
        contact.telephone = found.telephone || '';
        contact.email = found.email || '';
        contact.image = found.image || '';
        return;
    }
}

async function saveContact() {
    const payload = {
        id: contact.id,
        name: contact.name,
        telephone: contact.telephone,
        email: contact.email || '',
        image: contact.image
    };

    const res = await contactApi.updateContact(payload);
    const raw = localStorage.getItem('contacts');
    const list = raw ? JSON.parse(raw) : [];
    const idx = list.findIndex(c => Number(c.id) === Number(payload.id));
    if (idx >= 0) {
        list[idx] = { ...list[idx], name: payload.name, telephone: payload.telephone, email: payload.email, image: payload.image };
    } else {
        const id = payload.id || Date.now();
        list.push({ id, name: payload.name, telephone: payload.telephone, email: payload.email, image: payload.image });
    }
    localStorage.setItem('contacts', JSON.stringify(list));
    ElMessage.success('更新成功!');
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

            await updateContactWithNewImage(imageUrl);
            ElMessage({
                message: '头像上传成功',
                type: 'success',
                customClass: 'custom-message-style'
            });
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

async function updateContactWithNewImage(imageUrl) {
    if (!contact.id) {
        ElMessage.warning('无法更新头像：联系人ID不存在');
        return;
    }

    try {
        // 构建更新数据
        const updateData = {
            id: contact.id,
            name: contact.name || '',
            telephone: contact.telephone || '',
            email: contact.email || '',
            image: imageUrl
        };

        console.log('更新联系人数据:', updateData); // 调试

        // 调用更新接口
        const res = await contactApi.updateContact(updateData);

        // 解析响应
        let updatedContact = null;
        if (res) {
            if (res.data && res.data.id) {
                updatedContact = res.data;
            } else if (res.id) {
                updatedContact = res;
            } else if (res.data && res.data.data) {
                updatedContact = res.data.data;
            }
        }

        if (updatedContact) {
            console.log('联系人更新成功:', updatedContact);

            // 更新本地存储
            updateLocalStorage(updatedContact);

            // 可选：更新当前页面数据
            contact.name = updatedContact.name || contact.name;
            contact.telephone = updatedContact.telephone || contact.telephone;
            contact.email = updatedContact.email || contact.email;
            contact.image = updatedContact.image || imageUrl;

        } else {
            console.warn('联系人更新响应格式异常:', res);
            // 即使响应格式异常，也更新本地存储（使用我们自己的数据）
            updateLocalStorage({
                id: contact.id,
                name: contact.name,
                telephone: contact.telephone,
                email: contact.email,
                image: imageUrl
            });
        }
    } catch (error) {
        console.error('更新联系人失败:', error);
        ElMessage.warning('头像已上传，但更新联系人信息失败');
        // 即使失败，也更新本地存储（离线支持）
        updateLocalStorage({
            id: contact.id,
            name: contact.name,
            telephone: contact.telephone,
            email: contact.email,
            image: imageUrl
        });
    }
}

// 更新本地存储
function updateLocalStorage(contactData) {
    try {
        const raw = localStorage.getItem('contacts');
        const list = raw ? JSON.parse(raw) : [];
        const idx = list.findIndex(c => Number(c.id) === Number(contactData.id));

        if (idx >= 0) {
            // 更新现有联系人
            list[idx] = {
                ...list[idx],
                name: contactData.name || list[idx].name,
                telephone: contactData.telephone || list[idx].telephone,
                email: contactData.email || list[idx].email,
                image: contactData.image || list[idx].image
            };
        } else {
            // 添加新联系人
            list.push({
                id: contactData.id,
                name: contactData.name || '',
                telephone: contactData.telephone || '',
                email: contactData.email || '',
                image: contactData.image || ''
            });
        }

        localStorage.setItem('contacts', JSON.stringify(list));
        console.log('本地存储已更新');
    } catch (error) {
        console.error('更新本地存储失败:', error);
    }
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

onMounted(loadContact);
</script>

<style scoped>
/* 修复 ElMessage 样式 - 使用更高优先级 */
.el-message {
    min-height: auto !important;
    padding: 12px 16px !important;
    font-size: 14px !important;
    width: auto !important;
    max-width: 300px !important;
    border-radius: 8px !important;
}

.el-message__content {
    font-size: 14px !important;
    color: #ffffff !important;
    line-height: 1.4 !important;
    margin: 0 !important;
}

.el-message__icon {
    font-size: 20px !important;
    margin-right: 8px !important;
}

/* 修复成功状态的样式 */
.el-message--success .el-message__icon {
    color: #67c23a !important;
}

/* 修复

/* 修复 ElMessage 样式 */
.el-message {
    min-height: auto !important;
    padding: 12px 16px !important;
    font-size: 14px !important;
    width: auto !important;
}

.el-message__content {
    font-size: 14px !important;
    color: #ffffff !important;
    line-height: 1.4 !important;
}

/* 修复成功状态的样式 */
.el-message--success .el-message__icon {
    color: #67c23a !important;
}

/* 修复警告状态的样式 */
.el-message--warning .el-message__icon {
    color: #e6a23c !important;
}

/* 修复错误状态的样式 */
.el-message--error .el-message__icon {
    color: #f56c6c !important;
}




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
