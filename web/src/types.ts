export type Color = 'white' | 'black';
export type SideChoice = 'white' | 'black' | 'random';

export interface GameSettings {
	playerColor: Color;
	initialMs: number;
	incrementMs: number;
}

export type GameEndReason =
	| 'checkmate'
	| 'stalemate'
	| 'draw'
	| 'threefold'
	| 'insufficient'
	| 'timeout'
	| 'resign';

export interface GameResult {
	pgnResult: '1-0' | '0-1' | '1/2-1/2';
	reason: GameEndReason;
	winner?: Color;
	text: string;
}

export type EngineScore =
	| { kind: 'cp'; value: number }
	| { kind: 'mate'; value: number };

export interface EngineInfo {
	depth?: number;
	score?: EngineScore;
	nodes?: number;
	nps?: number;
}

export interface LastMove {
	from: string;
	to: string;
}

export const TIME_PRESETS: Record<string, { initialMs: number; incrementMs: number }> = {
	'1': { initialMs: 60_000, incrementMs: 0 },
	'1+1': { initialMs: 60_000, incrementMs: 1_000 },
	'3': { initialMs: 180_000, incrementMs: 0 },
	'3+1': { initialMs: 180_000, incrementMs: 1_000 },
	'5': { initialMs: 300_000, incrementMs: 0 },
};

export function resolveSide(choice: SideChoice): Color {
	if (choice === 'random') {
		return Math.random() < 0.5 ? 'white' : 'black';
	}
	return choice;
}

export function opponent(color: Color): Color {
	return color === 'white' ? 'black' : 'white';
}

export function formatClock(ms: number): string {
	const clamped = Math.max(0, ms);
	if (clamped < 10_000) {
		return (clamped / 1000).toFixed(1);
	}
	const totalSec = Math.floor(clamped / 1000);
	const minutes = Math.floor(totalSec / 60);
	const seconds = totalSec % 60;
	return `${minutes}:${seconds.toString().padStart(2, '0')}`;
}

export function timeControlTag(settings: GameSettings): string {
	const base = Math.round(settings.initialMs / 1000);
	if (settings.incrementMs > 0) {
		return `${base}+${Math.round(settings.incrementMs / 1000)}`;
	}
	return `${base}`;
}
