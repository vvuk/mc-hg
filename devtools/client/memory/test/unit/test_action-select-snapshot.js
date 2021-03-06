/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

/**
 * Tests the reducer responding to the action `selectSnapshot(snapshot)`
 */

let actions = require("devtools/client/memory/actions/snapshot");

function run_test() {
  run_next_test();
}

add_task(function *() {
  let front = new StubbedMemoryFront();
  yield front.attach();
  let store = Store();

  for (let i = 0; i < 5; i++) {
    store.dispatch(actions.takeSnapshot(front));
  }

  yield waitUntilState(store, ({ snapshots }) => snapshots.length === 5 && snapshots.every(isDone));

  ok(store.getState().snapshots[0].selected, "snapshot[0] selected by default");

  for (let i = 1; i < 5; i++) {
    do_print(`Selecting snapshot[${i}]`);
    store.dispatch(actions.selectSnapshot(store.getState().snapshots[i]));
    yield waitUntilState(store, ({ snapshots }) => snapshots[i].selected);

    let { snapshots } = store.getState();
    ok(snapshots[i].selected, `snapshot[${i}] selected`);
    equal(snapshots.filter(s => !s.selected).length, 4, "All other snapshots are unselected");
  }
});

function isDone (s) { return s.status === "done"; }
