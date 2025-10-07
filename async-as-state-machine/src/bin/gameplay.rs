use std::{
    cell::{Ref, RefCell, RefMut},
    collections::{HashMap, HashSet},
    future::Future,
    pin::Pin,
    task::{Context, Poll, RawWaker, RawWakerVTable, Waker},
};

use futures::future::poll_fn;

// ============================================================================
// Events
// ============================================================================

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
enum GameEvent {
    EntityDied(EntityId),
    AttackHit {
        attacker: EntityId,
        target: EntityId,
        damage: i32,
    },
    PlayerInput(InputAction),
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
enum InputAction {
    MoveUp,
    Attack,
}

struct EventSystem {
    listeners: HashMap<GameEvent, HashSet<CoroutineId>>,
    fired_events: Vec<GameEvent>,
}

impl EventSystem {
    fn new() -> Self {
        Self {
            listeners: HashMap::new(),
            fired_events: Vec::new(),
        }
    }

    fn register_listener(&mut self, event: GameEvent, coroutine_id: CoroutineId) {
        self.listeners
            .entry(event)
            .or_default()
            .insert(coroutine_id);
    }

    fn unregister_listener(&mut self, event: &GameEvent, coroutine_id: CoroutineId) {
        if let Some(listeners) = self.listeners.get_mut(event) {
            listeners.remove(&coroutine_id);
        }
    }

    fn fire_event(&mut self, event: GameEvent) {
        self.fired_events.push(event);
    }

    fn get_waiting_coroutines(&self, event: &GameEvent) -> Option<&HashSet<CoroutineId>> {
        self.listeners.get(event)
    }
}

// ============================================================================
// Game World & ECS-like Components
// ============================================================================

type EntityId = u32;
type CoroutineId = u32;

struct Entity {
    position: (f32, f32),
    velocity: (f32, f32),
    health: i32,
}

struct CoroutineState {
    needs_poll: bool,
}

struct CoroutineStorage {
    state: HashMap<CoroutineId, CoroutineState>,
    coroutines: HashMap<CoroutineId, Pin<Box<dyn Future<Output = ()>>>>,
    next_coroutine_id: CoroutineId,
}

impl CoroutineStorage {
    fn spawn_boxed_coroutine(&mut self, future: Pin<Box<dyn Future<Output = ()>>>) -> CoroutineId {
        let id = self.next_coroutine_id;
        self.next_coroutine_id += 1;
        self.state.insert(id, CoroutineState { needs_poll: true });
        self.coroutines.insert(id, future);
        id
    }
    fn spawn_coroutine<F>(&mut self, future: F) -> CoroutineId
    where
        F: Future<Output = ()> + 'static,
    {
        self.spawn_boxed_coroutine(Box::pin(future))
    }

    fn wake_coroutine(&mut self, id: CoroutineId) {
        if let Some(state) = self.state.get_mut(&id) {
            state.needs_poll = true;
        }
    }
}

pub struct GameWorld {
    entities: HashMap<EntityId, Entity>,
    events: EventSystem,
}

impl GameWorld {
    fn spawn_entity(&mut self, id: EntityId, x: f32, y: f32) {
        self.entities.insert(
            id,
            Entity {
                position: (x, y),
                velocity: (0.0, 0.0),
                health: 100,
            },
        );
    }
}

fn update_coroutines(coro_storage: &mut CoroutineStorage, world: &mut GameWorld) {
    // Process fired events and wake waiting coroutines
    let fired_events = world.events.fired_events.clone();
    if !fired_events.is_empty() {
        println!("Processing {} fired events", fired_events.len());
    }
    for event in &fired_events {
        if let Some(waiting) = world.events.get_waiting_coroutines(event) {
            for coroutine_id in waiting.clone() {
                println!("Waking coroutine {} for event {:?}", coroutine_id, event);
                coro_storage.wake_coroutine(coroutine_id);
            }
        }
    }
    world.events.fired_events.clear();

    let mut to_poll = Vec::new();
    for (id, state) in &mut coro_storage.state {
        if state.needs_poll {
            to_poll.push(*id);
        }
        state.needs_poll = false;
    }

    println!("Polling {:#?} coroutines", to_poll);

    for id in to_poll {
        let mut fut = coro_storage
            .coroutines
            .remove(&id)
            .expect("Coroutine not found");

        let waker_data = RefCell::new(WakerData {
            world,
            coro_storage,
            coroutine_id: id,
        });

        let waker = create_waker_in(&waker_data);
        let mut ctx = Context::from_waker(&waker);

        match waker_data.try_borrow_mut() {
            Ok(_) => println!("Waker data borrowed successfully"),
            Err(_) => println!("Waker data already borrowed"),
        }
        let ret = subsecond::call(|| fut.as_mut().poll(&mut ctx));
        std::thread::sleep(std::time::Duration::from_millis(50)); // Simulate some processing

        match ret {
            Poll::Ready(()) => {
                coro_storage.state.remove(&id);
            }
            Poll::Pending => {
                coro_storage.coroutines.insert(id, fut);
            }
        }
    }
}

// ============================================================================
// Waker implementation
// ============================================================================

pub struct WakerData<'a> {
    pub world: &'a mut GameWorld,
    coro_storage: &'a mut CoroutineStorage,
    pub coroutine_id: CoroutineId,
}

pub fn create_waker_in(waker_data: &RefCell<WakerData<'_>>) -> Waker {
    unsafe fn clone_waker(data: *const ()) -> RawWaker {
        RawWaker::new(data, &VTABLE)
    }

    unsafe fn wake_by_ref(data: *const ()) {
        let waker_data = unsafe { &*(data as *const RefCell<WakerData>) };

        let mut waker_data = waker_data.borrow_mut();
        let coroutine_id = waker_data.coroutine_id;
        waker_data.coro_storage.wake_coroutine(coroutine_id);
    }

    unsafe fn wake(data: *const ()) {
        unsafe { wake_by_ref(data) };
    }

    unsafe fn drop_waker(_data: *const ()) {}

    const VTABLE: RawWakerVTable = RawWakerVTable::new(clone_waker, wake, wake_by_ref, drop_waker);

    let raw = RawWaker::new(
        waker_data as *const RefCell<WakerData> as *const (),
        &VTABLE,
    );
    unsafe { Waker::from_raw(raw) }
}

pub async fn with_waker_data<F, R>(f: F) -> R
where
    F: for<'a, 'b> FnOnce(&'a WakerData<'b>) -> R,
{
    fn waker_data<'a, 'b>(cx: &'a Context<'_>) -> Ref<'a, WakerData<'b>> {
        unsafe { &*(cx.waker().data() as *const RefCell<WakerData>) }.borrow()
    }

    struct WithWakerData<F>(Option<F>);
    impl<F> Unpin for WithWakerData<F> {}

    impl<F, R> std::future::Future for WithWakerData<F>
    where
        F: for<'a, 'b> FnOnce(&'a WakerData<'b>) -> R,
    {
        type Output = R;

        fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
            let f = self
                .0
                .take()
                .expect("WithWakerData polled after completion");
            let mut waker_data = waker_data(cx);
            Poll::Ready(f(&mut waker_data))
        }
    }

    WithWakerData(Some(f)).await
}

async fn with_waker_data_mut<F, R>(f: F) -> R
where
    F: for<'a, 'b> FnOnce(&'a mut WakerData<'b>) -> R,
{
    fn waker_data_mut<'a, 'b>(cx: &'a Context<'_>) -> RefMut<'a, WakerData<'b>> {
        unsafe { &*(cx.waker().data() as *const RefCell<WakerData>) }.borrow_mut()
    }

    struct WithWakerDataMut<F>(Option<F>);
    impl<F> Unpin for WithWakerDataMut<F> {}

    impl<F, R> std::future::Future for WithWakerDataMut<F>
    where
        F: for<'a, 'b> FnOnce(&'a mut WakerData<'b>) -> R,
    {
        type Output = R;

        fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
            let f = self
                .0
                .take()
                .expect("WithWakerData polled after completion");
            let mut waker_data = waker_data_mut(cx);
            Poll::Ready(f(&mut waker_data))
        }
    }

    WithWakerDataMut(Some(f)).await
}
// Warning: this has a bug:
// loop {
//    wait_for_event(GameEvent::PlayerInput(InputAction::MoveUp)).await;
//  }
//    will never complete because the event is NOT unregistered when fired. thus, the coroutine
//    will never yield
//    + This only returns the 1st event fired!
async fn wait_for_event(event: GameEvent) {
    with_waker_data_mut(|wk| {
        wk.world
            .events
            .register_listener(event.clone(), wk.coroutine_id);
    })
    .await;

    loop {
        let event_fired = with_waker_data_mut(|wk| {
            if wk.world.events.fired_events.contains(&event) {
                wk.world.events.unregister_listener(&event, wk.coroutine_id);
                true
            } else {
                false
            }
        })
        .await;

        if event_fired {
            break;
        }
        poll_fn(|_cx| Poll::<()>::Pending).await;
    }
}

async fn fork<F>(future: F)
where
    F: Future<Output = ()> + 'static,
{
    with_waker_data_mut(|wk| {
        wk.coro_storage.spawn_coroutine(future);
    })
    .await
}

async fn fire_event(event: GameEvent) {
    with_waker_data_mut(|wk| {
        wk.world.events.fire_event(event);
    })
    .await
}

async fn set_velocity(entity_id: EntityId, vx: f32, vy: f32) {
    with_waker_data_mut(|wk| {
        if let Some(entity) = wk.world.entities.get_mut(&entity_id) {
            entity.velocity = (vx, vy);
        }
    })
    .await
}

async fn damage_entity(entity_id: EntityId, amount: i32) {
    with_waker_data_mut(|wk| {
        if let Some(entity) = wk.world.entities.get_mut(&entity_id) {
            entity.health -= amount;
            println!(
                "[Entity {}] took {} damage, health: {}",
                entity_id, amount, entity.health
            );

            if entity.health <= 0 {
                wk.world.events.fire_event(GameEvent::EntityDied(entity_id));
            }
        }
    })
    .await
}

async fn player_movement_controller(player_id: EntityId) {
    println!("[Player] Movement controller started");

    loop {
        wait_for_event(GameEvent::PlayerInput(InputAction::MoveUp)).await;
        println!("[Player] Moving up!");
        set_velocity(player_id, 0.0, 5.0).await;
        poll_fn(|_cx| Poll::<()>::Pending).await;
    }
}

fn main() {
    let mut w = GameWorld {
        entities: HashMap::new(),
        events: EventSystem::new(),
    };
    let mut coros = CoroutineStorage {
        coroutines: HashMap::new(),
        state: HashMap::new(),
        next_coroutine_id: 0,
    };

    // Setup entities
    w.spawn_entity(0, 0.0, 0.0); // player
    w.spawn_entity(1, 5.0, 5.0); // enemy 1
    w.spawn_entity(2, -5.0, 5.0); // enemy 2

    // Spawn gameplay coroutines
    coros.spawn_coroutine(player_movement_controller(0));

    // Simulate some player inputs
    coros.spawn_coroutine(async {
        fire_event(GameEvent::PlayerInput(InputAction::MoveUp)).await;
    });

    for i in 0.. {
        println!("--- Game Tick ---");
        if i % 20 == 0 {
            coros.spawn_coroutine(async {
                fire_event(GameEvent::PlayerInput(InputAction::MoveUp)).await;
            });
        }

        update_coroutines(&mut coros, &mut w);
        std::thread::sleep(std::time::Duration::from_millis(10));
    }

    println!("\n=== Final state ===");
    for (id, entity) in &w.entities {
        println!(
            "Entity {}: pos=({:.1}, {:.1}), vel=({:.1}, {:.1}), health={:.1}",
            id,
            entity.position.0,
            entity.position.1,
            entity.velocity.0,
            entity.velocity.1,
            entity.health
        );
    }
}
