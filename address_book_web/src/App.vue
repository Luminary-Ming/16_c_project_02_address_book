<script setup></script>

<template>
    <div id="app">
        <header class="app-header">

        </header>
        <main>
            <router-view />
        </main>
    </div>
</template>

<style scoped>
.app-header {
    padding: 16px;
    background: #ffffff;
    border-bottom: 1px solid #ffffff;
}

.app-header h1 {
    margin: 0;
    font-size: 18px;
}

main {
    padding: 16px;
}
</style>
