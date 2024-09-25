<script setup>
import { GridStack } from 'gridstack';
import NoCollisionSwapEngine from '@/gridstack/GridStackEngine.ts'
import { ref, onMounted, getCurrentInstance  } from 'vue'

const grid = ref(null);
const props = defineProps({
    row: {
        type: Number,
        default: 5,
    },
    column: {
        type: Number,
        default: 12,
    },
    sourceName: {
        type: String,
        default: undefined,
    }
})
const style = ref({
    width: props.column * 30 + "px"
})

let source = undefined;
if (props.sourceName === undefined || window[props.sourceName] === undefined) {
    source = {
        data: {
            layout: [
                { x: 0, y: 0, w: 2, h: 3, content: '<img src="src/assets/images/LunarArmor.webp"></img>', id: '1' },
                { x: 2, y: 3, w: 1, h: 3, content: '<img src="src/assets/images/FrostBite.webp"></img>', id: '2' },
                { x: 1, y: 3, w: 1, h: 1, content: '<img src="src/assets/images/QQicon.webp"></img>', id: '3' },
                { x: 4, y: 3, w: 2, h: 1, content: '<img src="src/assets/images/LuckyPiggybank.webp"></img>', id: '4' }
            ]
        },
        get Layout() {
            return this.data.layout;
        },
        set Layout(v) {
            this.data.layout = v;
        },
        set AddItemCallback(f) {

        }
    }
} else {
    source = window[props.sourceName]
}

onMounted(() => {
    let that = getCurrentInstance();
    grid.value = GridStack.init({
        row: props.row,
        column: props.column,
        margin: 0,
        cellHeight: '30px',
        disableResize: true,
        disableDrag: false,
        removable: '.trash',
        float: true,
        engineClass: NoCollisionSwapEngine,
        children: source.Layout,
        acceptWidgets: function (el) {
            return true;
        },
    }, that.proxy.$refs.stack);
    
    grid.value.on('dragstop', function (event, el) {
        source.Layout = grid.value.engine.save(false);
    });

    source.AddItemCallback = (item) => {
        grid.value.addWidget(item);
        source.Layout = grid.value.engine.save(false);
    }
})


</script>

<template>
 <div class="grid-stack" ref="stack" :style="style"></div>
</template>

<style lang="css" >
.grid-stack {
    background: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAB4AAAAeCAYAAAA7MK6iAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAACxEAAAsRAX9kX5EAAABUSURBVEhL7dexDQAxCARB7CZogJj+66ALqviXkWtYJzfSBSbZ2CsiPntgwu5uZ4Tunk04M+2MUFWzfd84hTEKYxTGKIxRGKMwRmGMwpi3X5h7A5n99JMZT7vzMhQAAAAASUVORK5CYII=');
    background-repeat: repeat;
    background-size: 30px 30px;
}


.grid-stack .grid-stack-item-content {
    display: flex;
    justify-content: center;
    padding: 1px;
    background-clip: content-box;
    overflow: visible !important;
    max-height: 100%;
}

.grid-stack .grid-stack-item-content img {
    width: auto;
    max-height: 100%;
}


.ui-draggable-dragging .grid-stack-item-content {
    background: red;
}

.grid-stack-item-removing {
    opacity: 0.5;
}
</style>