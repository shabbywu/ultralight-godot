<script setup>
import Inventory from './components/Inventory.vue'
import Trash from './components/Trash.vue'


</script>

<template>
  <div id="app">
    <Trash></Trash>
    <div class="control">
      <div>
        <p>Ground: </p>
        <Inventory :row="12" sourceName="__godot_ground" class="inventory-ground"></Inventory>
      </div>
      <div>
        <p>Bag: </p>
        <Inventory :row="6" sourceName="__godot_bag"  class="inventory-bag"></Inventory>
      </div>
      
    </div>
  </div>
</template>

<style scoped>
.control {
  display: flex;
  justify-content: space-around;
}
</style>
