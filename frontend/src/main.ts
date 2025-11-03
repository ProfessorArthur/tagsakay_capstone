import { createApp } from "vue";
import "./style.css";
import App from "./App.vue";
import router from "./router";

// Create and mount the app
const app = createApp(App);
app.use(router);
app.mount("#app");
