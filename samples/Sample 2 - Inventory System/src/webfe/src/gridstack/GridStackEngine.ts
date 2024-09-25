import { GridStackEngine, GridStackNode, GridStackMoveOpts } from 'gridstack';

class NoCollisionSwapEngine extends GridStackEngine {
  /** refined moveNodeCheck to disalbe auto swap when collided */
  override moveNodeCheck(node: GridStackNode, o: GridStackMoveOpts): boolean {
    // keep the same original X and Width and let base do it all...
    if (this.collide(node, o)) {
      return false;
    }
    return super.moveNodeCheck(node, o);
  }
}

export default NoCollisionSwapEngine;