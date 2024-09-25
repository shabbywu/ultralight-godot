import Vue from 'vue';
import App from './App.vue';
import * as VueWindow from '@hscmap/vue-window';
import 'gridstack/dist/gridstack.min.css';

import './assets/main.css';

Vue.use(VueWindow);

new Vue({
  render: (h) => h(App)
}).$mount('#app');
