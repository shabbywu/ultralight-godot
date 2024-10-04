import { GridStackEngine, GridStackNode, GridStackMoveOpts, Utils } from 'gridstack';


class NoCollisionSwapEngine extends GridStackEngine {
  /** refined moveNodeCheck to disalbe auto swap when collided */
  override moveNodeCheck(node: GridStackNode, o: GridStackMoveOpts): boolean {
    o.x = (o.x === undefined || o.x >= 0) ? o.x : 0;
    o.y = (o.y === undefined || o.y >= 0) ? o.y : 0;
    let collided = this.collide(node, o);
    if (collided) {
      return false;
    }
    
    let result = super.moveNodeCheck(node, o);
    console.log("[moveNodeCheck] in not collide result=", result, { x: o.x, y: o.w, w: o.w, h: o.h })
    return result;
  }

  _fixCollisions(node, nn = node, collide, opt = {}) {
    collide = collide || this.collide(node, nn); // REAL area collide for swap and skip if none...
    if (collide) {
      if (node._orig) {
        Utils.copyPos(node, node._orig)
      } else {
        console.log("[_fixCollisions] no tested branch", node)
        this.findEmptyPosition(node, this.nodes, undefined, undefined)
      }
    }
  }
}

export default NoCollisionSwapEngine;