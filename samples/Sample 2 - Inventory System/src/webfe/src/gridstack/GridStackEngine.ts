import { GridStackEngine, GridStackNode, GridStackMoveOpts, Utils } from 'gridstack';


class NoCollisionSwapEngine extends GridStackEngine {
  /** refined moveNodeCheck to disalbe auto swap when collided */
  override moveNodeCheck(node: GridStackNode, o: GridStackMoveOpts): boolean {
    if (this.collide(node, o)) {
      return false;
    }
    console.log("[moveNodeCheck] !collide", {x: o.x, y: o.w, w: o.w, h: o.h})
    return super.moveNodeCheck(node, o);
  }

  override swap(a: GridStackNode, b: GridStackNode): boolean | undefined {
    return super.swap(a, b);
  }

  _fixCollisions(node, nn = node, collide, opt = {}) {
    collide = collide || this.collide(node, nn); // REAL area collide for swap and skip if none...
    if (collide) {
      if (node._orig) {
        Utils.copyPos(node, node._orig)
      } else {
        this.findEmptyPosition(node, this.nodes, this.column, undefined)
      }
    }
  }
}

export default NoCollisionSwapEngine;