import type { Color, EngineInfo, EngineScore } from './types';

export interface PhotonModule {
	ccall: (
		name: string,
		returnType: string | null,
		argTypes: string[],
		args: unknown[],
	) => unknown;
}

export type PhotonFactory = (options: {
	locateFile?: (path: string, prefix: string) => string;
	print?: (text: string) => void;
	printErr?: (text: string) => void;
}) => Promise<PhotonModule>;

export interface GoClocks {
	wtime: number;
	btime: number;
	winc: number;
	binc: number;
}

export interface BestMove {
	move: string;
	ponder?: string;
}

export type ParsedInfo = EngineInfo;

type LineHandler = (line: string) => void;

export class PhotonEngine {
	private readonly handlers = new Set<LineHandler>();
	private readonly unread: string[] = [];

	constructor(private readonly module: PhotonModule) {}

	static async load(): Promise<PhotonEngine> {
		const engineDir = new URL(`${import.meta.env.BASE_URL}engine/`, window.location.href);
		const mod = (await import(/* @vite-ignore */ new URL('photon.js', engineDir).href)) as {
			default: PhotonFactory;
		};
		const pending: string[] = [];
		let engine: PhotonEngine | undefined;

		const module = await mod.default({
			locateFile: (path) => new URL(path, engineDir).href,
			print: (text) => {
				if (engine) {
					engine.dispatch(text);
				} else {
					pending.push(text);
				}
			},
			printErr: (text) => {
				console.error(text);
			},
		});

		engine = new PhotonEngine(module);
		for (const line of pending) {
			engine.dispatch(line);
		}

		const uciok = engine.waitFor((line) => line === 'uciok', 15_000);
		engine.send('uci');
		await uciok;
		const readyok = engine.waitFor((line) => line === 'readyok', 15_000);
		engine.send('isready');
		await readyok;
		return engine;
	}

	onLine(handler: LineHandler): () => void {
		this.handlers.add(handler);
		return () => this.handlers.delete(handler);
	}

	send(command: string): void {
		this.module.ccall('photon_uci_cmd', null, ['string'], [command]);
	}

	go(clocks: GoClocks, ponder = false): void {
		const parts = ['go'];
		if (ponder) {
			parts.push('ponder');
		}
		parts.push(
			'wtime',
			String(Math.max(1, Math.floor(clocks.wtime))),
			'btime',
			String(Math.max(1, Math.floor(clocks.btime))),
			'winc',
			String(Math.max(0, Math.floor(clocks.winc))),
			'binc',
			String(Math.max(0, Math.floor(clocks.binc))),
		);
		this.send(parts.join(' '));
	}

	stop(): void {
		this.send('stop');
	}

	ponderhit(): void {
		this.send('ponderhit');
	}

	private dispatch(text: string): void {
		for (const line of text.split(/\r?\n/)) {
			const trimmed = line.trim();
			if (!trimmed) {
				continue;
			}
			if (this.handlers.size === 0) {
				this.unread.push(trimmed);
				continue;
			}
			for (const handler of this.handlers) {
				handler(trimmed);
			}
		}
	}

	private waitFor(pred: (line: string) => boolean, timeoutMs: number): Promise<string> {
		const queued = this.unread.findIndex(pred);
		if (queued >= 0) {
			return Promise.resolve(this.unread.splice(queued, 1)[0]);
		}
		return new Promise((resolve, reject) => {
			const timer = window.setTimeout(() => {
				unsub();
				reject(new Error('Timed out waiting for engine response'));
			}, timeoutMs);
			const unsub = this.onLine((line) => {
				if (pred(line)) {
					window.clearTimeout(timer);
					unsub();
					resolve(line);
				}
			});
		});
	}
}

export function parseBestmove(line: string): BestMove | null {
	if (!line.startsWith('bestmove ')) {
		return null;
	}
	const parts = line.split(/\s+/);
	const move = parts[1];
	if (!move || move === '(none)') {
		return null;
	}
	const ponderIdx = parts.indexOf('ponder');
	const ponder = ponderIdx >= 0 ? parts[ponderIdx + 1] : undefined;
	return { move, ponder };
}

export function parseInfo(line: string): ParsedInfo | null {
	if (!line.startsWith('info ')) {
		return null;
	}
	const parts = line.split(/\s+/);
	const info: ParsedInfo = {};
	for (let i = 1; i < parts.length; i++) {
		const token = parts[i];
		if (token === 'depth' && parts[i + 1]) {
			info.depth = Number(parts[++i]);
		} else if (token === 'nodes' && parts[i + 1]) {
			info.nodes = Number(parts[++i]);
		} else if (token === 'nps' && parts[i + 1]) {
			info.nps = Number(parts[++i]);
		} else if (token === 'score' && parts[i + 1]) {
			const kind = parts[++i];
			const value = parts[++i];
			if (kind === 'cp' && value !== undefined) {
				info.score = { kind: 'cp', value: Number(value) };
			} else if (kind === 'mate' && value !== undefined) {
				info.score = { kind: 'mate', value: Number(value) };
			}
		} else if (token === 'pv') {
			break;
		}
	}
	return info;
}

/** Format a UCI side-to-move score as White-positive (absolute). */
export function formatAbsoluteScore(score: EngineScore, sideToMove: Color): string {
	const whiteSign = sideToMove === 'black' ? -1 : 1;
	if (score.kind === 'cp') {
		const cp = score.value * whiteSign;
		return `${cp >= 0 ? '+' : ''}${(cp / 100).toFixed(2)}`;
	}
	const mate = score.value * whiteSign;
	return `M${mate}`;
}
