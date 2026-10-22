import Vue from "vue";
import App from "./App.vue";
import { createReliquary } from "./game/session";
import "./assets/main.css";

const backend = window.inventory?.backend;
if (!backend) {
  document.getElementById("app").textContent = "Open this sample in Godot .NET and run the scene.";
} else {
  const session = createReliquary({
    backend,
    toValue: (value) => value,
    subscribe(listener) {
      const handler = (event) => listener(event.detail);
      window.addEventListener("inventory-changed", handler);
      return () => window.removeEventListener("inventory-changed", handler);
    },
  });
  window.Reliquary = session;
  new Vue({
    render: (h) => h(App, { props: { session, hostName: backend.host_name() } }),
  }).$mount("#app");
}
