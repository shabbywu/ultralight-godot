<script setup>
import { v4 as uuidv4 } from 'uuid';
import { GridStack, Utils } from 'gridstack';
import NoCollisionSwapEngine from '@/gridstack/GridStackEngine.ts'
import { ref, onMounted, getCurrentInstance } from 'vue'

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


const cellSize = 30;
const style = ref({
    width: props.column * cellSize + "px",
    background: "url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAB4AAAAeCAYAAAA7MK6iAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAACxEAAAsRAX9kX5EAAABUSURBVEhL7dexDQAxCARB7CZogJj+66ALqviXkWtYJzfSBSbZ2CsiPntgwu5uZ4Tunk04M+2MUFWzfd84hTEKYxTGKIxRGKMwRmGMwpi3X5h7A5n99JMZT7vzMhQAAAAASUVORK5CYII=')",
    "background-repeat": "repeat",
    "background-size": `${cellSize}px ${cellSize}px`,
})

let source = undefined;
if (props.sourceName === undefined || window[props.sourceName] === undefined) {
    source = {
        data: {
            layout: [
                { x: 0, y: 0, w: 2, h: 3, content: '<img src="src/assets/images/LunarArmor.webp"></img>', id: uuidv4() },
                { x: 2, y: 3, w: 1, h: 3, content: '<img src="src/assets/images/FrostBite.webp"></img>', id: uuidv4() },
                { x: 1, y: 3, w: 1, h: 1, content: '<img src="src/assets/images/QQicon.webp"></img>', id: uuidv4() },
                { x: 4, y: 3, w: 2, h: 1, content: '<img src="src/assets/images/LuckyPiggybank.webp"></img>', id: uuidv4() }
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


const rotating = ref(null);
function setupRotatedNode(node, content, updating) {
    content = content || node.el.querySelector('.grid-stack-item-content');
    if (updating !== undefined) {
        const _orig = node._orig;
        const currentGrid = node.grid;
        const CurrentEngine = currentGrid.engine;
        CurrentEngine.findEmptyPosition(node, CurrentEngine.nodes.filter(n => node !== n), CurrentEngine.column)
        currentGrid.update(node.el, updating);
        node._orig = _orig;
        Utils.swap(node._orig, 'w', 'h');
        console.log("_orig: ", {...node._orig}, {...updating})
    }

    let rotateDeg = node.rotateDeg || 0;
    let w = node.w;
    let h = node.h;

    content.setAttribute("rotate", rotateDeg);
    if (updating !== undefined) {
        content.setAttribute("updating", 1);
    }

    content.style.width = `${h * cellSize}px`;
    content.style.position = `relative`;
}

function _keyEvent(e) {
    if (e.key === 'r' || e.key === 'R') {
        if (rotating.value == null) return;
        let node = rotating.value.el.gridstackNode;
        let rotateDeg = node.rotateDeg || 0;
        let w = node.h;
        let h = node.w;
        rotateDeg += 90;
        rotateDeg %= 360;
        setupRotatedNode(node, rotating.value.content, {
            rotateDeg,
            w,
            h
        });
    }
}


onMounted(() => {
    let that = getCurrentInstance();
    let keyEvent = _keyEvent.bind(that);
    grid.value = GridStack.init({
        row: props.row,
        column: props.column,
        margin: 0,
        cellHeight: `${cellSize}px`,
        disableResize: true,
        disableDrag: false,
        removable: '.trash',
        float: true,
        engineClass: NoCollisionSwapEngine,
        acceptWidgets: function (el) {
            return true;
        },
    }, that.proxy.$refs.stack);

    grid.value.on('added', function (event, items) {
        items.forEach(node => {
            setupRotatedNode(node);
        });
    });

    grid.value.on('change', function (event, items) {
        source.Layout = grid.value.engine.save(false);
    });

    grid.value.on('dragstart', function (event, el) {
        rotating.value = {
            el,
            content: el.querySelector('.grid-stack-item-content'),
            node: el.gridstackNode,
        };

        // switch width and height to fixup rotate(90deg) or rotate(270deg)
        if (el.gridstackNode.rotateDeg == 90 || el.gridstackNode.rotateDeg == 270) {
            let width = el.style.width;
            let height = el.style.height;
            el.style.width = height;
            el.style.height = width;
        }
        document.addEventListener('keydown', keyEvent);
    });

    grid.value.on('dragstop', function (event, el) {
        rotating.value.content.setAttribute("updating", 0);
        rotating.value = null;
        el.style.marginTop = null;
        document.removeEventListener('keydown', keyEvent);
    });

    grid.value.on('removed', function (event, items) {
        rotating.value.content.setAttribute("updating", 0);
        rotating.value = null;
        document.removeEventListener('keydown', keyEvent);
    })

    grid.value.load(source.Layout);

    source.AddItemCallback = (item, force) => {
        if (grid.value.engine.willItFit(item) || force) {
            grid.value.addWidget(item);
            source.Layout = grid.value.engine.save(false);
            return true;
        }
        return false;
    }

    window.grid = grid.value;
    window.engine = grid.value.engine;
})


</script>

<template>
    <div class="grid-stack" ref="stack" :style="style"></div>
</template>

<style lang="scss">
.grid-stack .grid-stack-item-content {
    display: flex;
    justify-content: center;
    align-items: center;
    padding: 1px;
    background-clip: content-box;
    overflow: visible !important;

    max-height: 100%;
}

.grid-stack {
    .grid-stack-item {
        display: flex;
        justify-content: center;
        align-items: center;
    }

    .grid-stack-item-content[rotate="0"] {
        rotate: 0deg;
    }

    .grid-stack-item-content[rotate="90"] {
        rotate: 90deg;
    }

    .grid-stack-item-content[rotate="180"] {
        rotate: 180deg;
    }

    .grid-stack-item-content[rotate="270"] {
        rotate: 270deg;
    }
}


.grid-stack .grid-stack-item-content img {
    max-width: 100% !important;
    max-height: 100% !important;
    width: fit-content;
    height: fit-content;
}

.ui-draggable-dragging .grid-stack-item-content {
    background: red;
}

/* .ui-draggable-dragging .grid-stack-item-content[updating="0"] {
    rotate: unset !important;
} */

.grid-stack-item-removing {
    opacity: 0.5;
}
</style>