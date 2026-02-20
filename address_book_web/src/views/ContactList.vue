<template>
    <div class="contact-list mobile">
        <div class="title-row">
            <div class="title">联系人</div>
            <button class="add-btn" @click="goAdd">+</button>
        </div>

        <div class="search">
            <input type="text" v-model="searchQuery" placeholder="在联系人中搜索" />
        </div>

        <div class="list-wrap">
            <template v-for="(items, letter) in groupedContacts">
                <div class="group" v-if="items.length" :key="letter">
                    <div class="group-header" :id="`grp-${letter === '#' ? 'sharp' : letter}`">{{ letter }}</div>
                    <div class="item" v-for="c in items" :key="c.id">
                        <div class="avatar">{{ c.initial }}</div>
                        <div class="info">
                            <div class="name">{{ c.name }}</div>
                            <div class="telephone">{{ c.telephone }}</div>
                        </div>
                        <div class="actions">
                            <button class="btn green" @click="callContact(c)">拨打</button>
                            <button class="btn" @click="editContact(c)">修改</button>
                            <button class="btn danger" @click="deleteContact(c.id)">删除</button>
                        </div>
                    </div>
                </div>
            </template>
        </div>

        <div class="alpha-index">
            <div v-for="l in letters" :key="l" class="alpha" @click="scrollTo(l)">{{ l }}</div>
        </div>

        <div class="dial-fab" @click="toggleDialer" title="拨号">
            <div class="fab-icon">📞</div>
        </div>


        <div v-if="showDialer" class="dial-overlay" @click.self="closeDialer">
            <div class="dial-panel">
                <div class="dial-display">{{ formattedDialNumber }}</div>
                <div class="dial-keypad">
                    <button v-for="k in keypad" :key="k" class="key" @click="appendDigit(k)">{{ k }}</button>
                </div>
                <div class="dial-actions">
                    <button class="btn" @click="clearDial">清除</button>
                    <button class="btn green" @click="callNumber">拨打</button>
                    <button class="btn" @click="backspace">⌫</button>

                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, computed, onMounted, onBeforeUnmount } from 'vue';
import { useRouter } from 'vue-router';
import { contactApi } from '@/api/contact';

const router = useRouter();

const searchQuery = ref('');
const letters = ['#', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'];
const contacts = ref([]);
const showDialer = ref(false);
const dialNumber = ref('');
const keypad = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '0', '#'];

const formattedDialNumber = computed(() => {
    // simple grouping for readability
    const v = dialNumber.value.replace(/\D/g, '');
    if (!v) return '';
    return v.replace(/(\d{3})(?=\d)/g, '$1 ');
});


const filteredContacts = computed(() => {
    const q = searchQuery.value.trim().toLowerCase();
    return contacts.value
        .filter(c => !q || c.name.toLowerCase().includes(q) || (c.telephone || '').includes(q))
        .slice()
        .sort((a, b) => {
            if (a.initial === b.initial) return a.name.localeCompare(b.name);
            if (a.initial === '#') return 1;
            if (b.initial === '#') return -1;
            return a.initial.localeCompare(b.initial);
        });
});

const groupedContacts = computed(() => {
    const groups = {};
    letters.forEach(l => (groups[l] = []));
    for (const c of filteredContacts.value) {
        const key = (c.initial && letters.includes(c.initial)) ? c.initial : '#';
        groups[key].push(c);
    }
    return groups;
});

async function fetchContacts() {
    const res = await contactApi.getContacts();
    let list = null;
    if (!res) list = null;

    const parsedData = res.data;  // 解析 JSON 字符串
    list = parsedData.data;

    if (Array.isArray(list) && list.length) {
        contacts.value = list.map(c => ({
            id: c.id,
            name: c.name,
            telephone: c.telephone || '',
            initial: c.initial || (c.name ? c.name[0].toUpperCase() : '#'),
            image: c.image || '',
            email: c.email || ''
        }));
        localStorage.setItem('contacts', JSON.stringify(contacts.value));
        return;
    }

    const raw = localStorage.getItem('contacts');
    if (raw) {
        try {
            contacts.value = JSON.parse(raw);
        } catch (e) {
            console.warn('failed parse contacts', e);
        }
    } else {
        localStorage.setItem('contacts', JSON.stringify(contacts.value));
    }
}

function editContact(contact) {
    router.push({ name: 'EditContact', params: { id: contact.id } });
}

async function deleteContact(id) {
    contacts.value = contacts.value.filter(c => c.id !== id);
    localStorage.setItem('contacts', JSON.stringify(contacts.value));
    try { await contactApi.deleteContact(id); } catch (e) { console.warn('delete API failed', e); }
}

async function callContact(contact) {
    if (!contact || !contact.telephone) {
        alert('无可用电话号码');
        return;
    }
    try {
        await contactApi.callContact({ telephone: contact.telephone });
        alert('拨打请求已发送');
    } catch (e) {
        console.warn('call API failed', e);
        alert('拨打请求失败');
    }
}

function toggleDialer() {
    showDialer.value = !showDialer.value;
}

function closeDialer() {
    showDialer.value = false;
}

function appendDigit(d) {
    dialNumber.value += d;
}

function backspace() {
    dialNumber.value = dialNumber.value.slice(0, -1);
}

function clearDial() {
    dialNumber.value = '';
}

async function callNumber() {
    const num = dialNumber.value.replace(/\s+/g, '');
    if (!num) {
        alert('请输入要拨打的号码');
        return;
    }
    try {
        await contactApi.callContact({ telephone: num });
        alert('拨打请求已发送');
        dialNumber.value = '';
        showDialer.value = false;
    } catch (e) {
        console.warn('call API failed', e);
        alert('拨打请求失败');
    }
}

function scrollTo(letter) {
    const key = letter === '#' ? 'sharp' : letter;
    const el = document.getElementById(`grp-${key}`);
    if (el) el.scrollIntoView({ behavior: 'smooth', block: 'start' });
}

function goAdd() { router.push({ name: 'AddContact' }); }

onMounted(fetchContacts,);

const onKeydown = (e) => {
    if (!showDialer.value) return;
    const k = e.key;
    if (/^[0-9]$/.test(k) || k === '*' || k === '#') {
        // append digit
        appendDigit(k);
        e.preventDefault();
        return;
    }
    if (k === 'Backspace') {
        backspace();
        e.preventDefault();
        return;
    }
    if (k === 'Enter') {
        callNumber();
        e.preventDefault();
        return;
    }
    if (k === 'Escape') {
        closeDialer();
        e.preventDefault();
        return;
    }
};

onMounted(() => {
    // initial fetch
    fetchContacts();
    window.addEventListener('keydown', onKeydown);
});

onBeforeUnmount(() => {
    window.removeEventListener('keydown', onKeydown);
});

</script>
<style scoped>
.mobile {
    max-width: 480px;
    margin: 0 auto;
    background: #0f0f10;
    color: #fff;
    min-height: calc(100vh - 60px);
    position: relative;
    padding-bottom: 60px;
}


.title {
    font-size: 28px;
    font-weight: 700;
    padding: 18px 16px;
}

.title-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
}

.add-btn {
    background: #22c55e;
    color: #fff;
    border: none;
    width: 36px;
    height: 36px;
    border-radius: 6px;
    font-size: 20px;
    margin-right: 20px;
    cursor: pointer;
}

.search {
    padding: 0 16px 12px;
}

.search input {
    width: 95%;
    padding: 10px 12px;
    border-radius: 6px;
    border: 1px solid rgba(255, 255, 255, 0.08);
    background: rgba(255, 255, 255, 0.03);
    color: #ddd;
}

.list-wrap {
    padding-bottom: 80px;
}

.group-header {
    padding: 12px 16px;
    font-size: 14px;
    color: #9aa0a6;
    border-top: 1px solid rgba(255, 255, 255, 0.03);
}

.item {
    display: flex;
    align-items: center;
    padding: 12px 16px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.03);
}

.avatar {
    width: 44px;
    height: 44px;
    background: #22c55e;
    display: flex;
    align-items: center;
    justify-content: center;
    color: #fff;
    font-weight: 700;
    border-radius: 4px;
    margin-right: 12px;
}

.info .name {
    font-size: 18px;
}

.info .telephone {
    font-size: 16px;
    color: #9aa0a6;
}

.actions {
    margin-left: auto;
    display: flex;
    gap: 8px;
}

.btn {
    background: rgba(0, 124, 241, 0.555);
    color: #fff;
    border: none;
    padding: 6px 8px;
    border-radius: 4px;
    cursor: pointer;
}

.btn.green {
    background: #19793c;
}

.btn.danger {
    background: rgb(196, 5, 5);
    margin-right: 20px;
}

.alpha-index {
    position: absolute;
    right: 8px;
    top: 120px;
    display: flex;
    flex-direction: column;
    gap: 6px;
    z-index: 50;
    pointer-events: auto;
    cursor: pointer;
}

.dial-fab {
    position: sticky;
    left: 20px;
    bottom: 50px;
    width: 56px;
    height: 56px;
    border-radius: 50%;
    background: #22c55e;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    z-index: 150;
    box-shadow: 0 6px 12px rgba(0, 0, 0, 0.3);
    margin-left: 20px;
}

.fab-icon {
    font-size: 22px;
}

.dial-overlay {
    position: fixed;
    left: 0;
    right: 0;
    top: 0;
    bottom: 0;
    background: rgba(0, 0, 0, 0.4);
    display: flex;
    align-items: flex-end;
    justify-content: center;
    z-index: 200;
}

.dial-panel {
    width: 100%;
    max-width: 480px;
    background: #0b0b0b;
    padding: 12px;
    border-top-left-radius: 12px;
    border-top-right-radius: 12px;
}

.dial-display {
    font-size: 26px;
    color: #fff;
    padding: 12px 8px;
    text-align: center;
}

.dial-keypad {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 8px;
    padding: 8px;
}

.dial-keypad .key {
    background: rgba(255, 255, 255, 0.04);
    color: #fff;
    padding: 18px 0;
    border-radius: 8px;
    font-size: 18px;
    border: none;
    cursor: pointer;
}

.dial-actions {
    display: flex;
    gap: 10px;
    padding: 10px;
    justify-content: space-between;
}

.dial-actions .btn {
    flex: 1;
}
</style>