export enum MBTNS {
    MBTN_LEFT = 0,
    MBTN_RIGHT = 1,
    MBTN_MID = 2
}

export interface WaymoLoopConfig {
    /** Maximum number of commands in the queue */
    maxCommands?: number;
    
    /** Keyboard layout (e.g., "us", "gb", "fr") */
    kbdLayout?: string;
    
    /** Cooldown between each action in milliseconds */
    actionCooldownMs?: number;
}

export class WaymoLoop {
    /** Creates a new Waymo event loop with optional configuration */
    constructor(config?: WaymoLoopConfig);
    
    /** Moves mouse to coordinates or relative to current position */
    moveMouse(x: number, y: number, relative: boolean): void;
    
    /** Clicks a specific mouse button multiple times */
    clickMouse(btn: MBTNS, clicks: number, holdMs: number): void;
    
    /** Sets the state of a mouse button (down or up) */
    pressMouse(btn: MBTNS, down: boolean): void;
    
    /** Presses a single key down or releases it */
    pressKey(key: string, down: boolean, intervalMs?: number): void;
    
    /** Holds a key down for a specified duration */
    holdKey(key: string, holdMs: number, intervalMs?: number): void;
    
    /** Types a full string of text */
    type(text: string, intervalMs?: number): void;
}
