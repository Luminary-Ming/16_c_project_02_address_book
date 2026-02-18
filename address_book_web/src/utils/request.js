import axios from 'axios';

const service = axios.create({
    baseURL: 'http://192.168.2.227:8080/api',
    timeout: 10000,
});

export default service;