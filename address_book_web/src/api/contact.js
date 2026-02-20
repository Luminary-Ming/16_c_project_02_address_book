import request from "@/utils/request";

export const contactApi = {
    getContacts() {
        return request({
            url: `/contacts`,
            method: "get",
        });
    },
    getContact(id) { 
        return request({
            url: `/contacts/${id}`,
            method: "get",
        });
    },
    addContact(contactData) { 
        return request({
            url: `/contacts`,
            method: "post",
            data: contactData,
        });
    },
    updateContact(contactData) { 
        return request({
            url: `/contacts`,
            method: "put",
            data: contactData,
        });
    },
    deleteContact(id) { 
        return request({
            url: `/contacts/${id}`,
            method: "delete",
        });
    },
    callContact(payload) {
        return request({
            url: `/contacts/call`,
            method: "post",
            data: payload,
        });
    },
};