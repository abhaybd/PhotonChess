import type { Color } from './types';

export class GameClock {
	remaining: Record<Color, number>;
	readonly incrementMs: number;
	private running: Color | null = null;
	private lastTick = 0;
	private intervalId: ReturnType<typeof setInterval> | null = null;
	private flagged = false;

	constructor(
		initialMs: number,
		incrementMs: number,
		private readonly onFlag: (color: Color) => void,
		private readonly onTick: () => void,
	) {
		this.remaining = { white: initialMs, black: initialMs };
		this.incrementMs = incrementMs;
	}

	start(color: Color): void {
		this.sync();
		this.running = color;
		this.lastTick = performance.now();
		this.arm();
	}

	onMove(mover: Color): void {
		this.sync();
		if (this.flagged) {
			return;
		}
		this.remaining[mover] += this.incrementMs;
		this.running = mover === 'white' ? 'black' : 'white';
		this.lastTick = performance.now();
		this.arm();
	}

	stop(): void {
		this.sync();
		this.running = null;
		this.disarm();
	}

	dispose(): void {
		this.running = null;
		this.disarm();
	}

	private arm(): void {
		if (this.intervalId !== null) {
			return;
		}
		this.intervalId = setInterval(() => {
			this.sync();
			this.onTick();
		}, 50);
	}

	private disarm(): void {
		if (this.intervalId !== null) {
			clearInterval(this.intervalId);
			this.intervalId = null;
		}
	}

	private sync(): void {
		if (!this.running || this.flagged) {
			return;
		}
		const now = performance.now();
		this.remaining[this.running] -= now - this.lastTick;
		this.lastTick = now;
		if (this.remaining[this.running] <= 0) {
			this.remaining[this.running] = 0;
			const color = this.running;
			this.running = null;
			this.flagged = true;
			this.disarm();
			this.onFlag(color);
		}
	}
}
