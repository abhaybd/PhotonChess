import { Chess, type Square } from 'chess.js';
import { GameClock } from './clock';
import {
	parseBestmove,
	parseInfo,
	type PhotonEngine,
} from './engine';
import { opponent, type Color, type EngineInfo, type GameResult, type GameSettings, type LastMove } from './types';

export type DestMap = Map<string, string[]>;

export interface GameSnapshot {
	fen: string;
	lastMove: LastMove | null;
	dests: DestMap;
	playerColor: Color;
	turn: Color;
	check: boolean;
	movable: boolean;
	clocks: Record<Color, number>;
	engineInfo: EngineInfo | null;
	thinking: boolean;
	over: GameResult | null;
	sans: string[];
}

export type GameListener = (snapshot: GameSnapshot) => void;

function uciMoves(chess: Chess): string[] {
	return chess.history({ verbose: true }).map((move) => move.lan);
}

function applyUci(chess: Chess, uci: string) {
	try {
		return chess.move({
			from: uci.slice(0, 2),
			to: uci.slice(2, 4),
			promotion: uci.length > 4 ? uci[4] : undefined,
		});
	} catch {
		return null;
	}
}

function destsFrom(chess: Chess): DestMap {
	const dests: DestMap = new Map();
	if (chess.isGameOver()) {
		return dests;
	}
	for (const move of chess.moves({ verbose: true })) {
		const from = dests.get(move.from) ?? [];
		from.push(move.to);
		dests.set(move.from, from);
	}
	return dests;
}

function turnColor(chess: Chess): Color {
	return chess.turn() === 'w' ? 'white' : 'black';
}

export class GameController {
	readonly chess = new Chess();
	readonly clock: GameClock;
	private lastMove: LastMove | null = null;
	private engineInfo: EngineInfo | null = null;
	private over: GameResult | null = null;
	private pondering = false;
	private ponderMove: string | null = null;
	private awaitBest: 'play' | 'discard' | null = null;
	private readonly listeners = new Set<GameListener>();
	private readonly unsub: () => void;

	constructor(
		private readonly engine: PhotonEngine,
		readonly settings: GameSettings,
	) {
		this.clock = new GameClock(
			settings.initialMs,
			settings.incrementMs,
			(color) => this.onFlag(color),
			() => this.emit(),
		);
		this.unsub = engine.onLine((line) => {
			window.setTimeout(() => this.onEngineLine(line), 0);
		});
	}

	start(): void {
		this.engine.send('ucinewgame');
		this.engine.send('position startpos');
		this.clock.start('white');
		if (this.settings.playerColor === 'black') {
			this.requestEngineMove();
		}
		this.emit();
	}

	subscribe(listener: GameListener): () => void {
		this.listeners.add(listener);
		listener(this.snapshot());
		return () => this.listeners.delete(listener);
	}

	snapshot(): GameSnapshot {
		const turn = turnColor(this.chess);
		return {
			fen: this.chess.fen(),
			lastMove: this.lastMove,
			dests: destsFrom(this.chess),
			playerColor: this.settings.playerColor,
			turn,
			check: this.chess.isCheck(),
			movable: !this.over && turn === this.settings.playerColor,
			clocks: { ...this.clock.remaining },
			engineInfo: this.engineInfo,
			thinking: this.awaitBest === 'play',
			over: this.over,
			sans: this.chess.history(),
		};
	}

	needsPromotion(from: string, to: string): boolean {
		const piece = this.chess.get(from as Square);
		if (!piece || piece.type !== 'p') {
			return false;
		}
		const rank = to[1];
		return rank === '8' || rank === '1';
	}

	playPlayerMove(from: string, to: string, promotion?: string): boolean {
		if (this.over || turnColor(this.chess) !== this.settings.playerColor) {
			return false;
		}
		const played = this.chess.move({ from, to, promotion });
		if (!played) {
			return false;
		}
		this.lastMove = { from, to };
		this.clock.onMove(this.settings.playerColor);
		if (this.checkTerminal()) {
			this.emit();
			return true;
		}

		const uci = from + to + (promotion ?? '');
		if (this.pondering) {
			if (this.ponderMove && uci === this.ponderMove) {
				this.pondering = false;
				this.ponderMove = null;
				this.awaitBest = 'play';
				this.engine.ponderhit();
			} else {
				this.pondering = false;
				this.ponderMove = null;
				this.awaitBest = 'discard';
				this.engine.stop();
			}
		} else {
			this.requestEngineMove();
		}
		this.emit();
		return true;
	}

	resign(): void {
		if (this.over) {
			return;
		}
		this.haltEngine();
		this.endGame({
			pgnResult: this.settings.playerColor === 'white' ? '0-1' : '1-0',
			reason: 'resign',
			winner: opponent(this.settings.playerColor),
			text: `${capitalize(this.settings.playerColor)} resigned`,
		});
	}

	dispose(): void {
		this.haltEngine();
		this.clock.dispose();
		this.unsub();
	}

	private requestEngineMove(): void {
		this.sendPosition();
		this.awaitBest = 'play';
		this.engine.go(this.goClocks(), false);
	}

	private startPonder(): void {
		if (!this.ponderMove) {
			return;
		}
		// UCI: the last move in "position" is the predicted opponent move.
		this.sendPosition(this.ponderMove);
		this.pondering = true;
		this.awaitBest = null;
		this.engine.go(this.goClocks(), true);
	}

	private sendPosition(extraMove?: string): void {
		const moves = uciMoves(this.chess);
		if (extraMove) {
			moves.push(extraMove);
		}
		this.engine.send(
			moves.length === 0 ? 'position startpos' : `position startpos moves ${moves.join(' ')}`,
		);
	}

	private goClocks() {
		return {
			wtime: Math.max(1, this.clock.remaining.white),
			btime: Math.max(1, this.clock.remaining.black),
			winc: this.settings.incrementMs,
			binc: this.settings.incrementMs,
		};
	}

	private onEngineLine(line: string): void {
		if (this.over) {
			return;
		}
		const info = parseInfo(line);
		if (info) {
			this.engineInfo = { ...this.engineInfo, ...info };
			this.emit();
			return;
		}
		const best = parseBestmove(line);
		if (!best) {
			return;
		}
		if (this.awaitBest === 'discard') {
			this.requestEngineMove();
			this.emit();
			return;
		}
		if (this.awaitBest !== 'play') {
			return;
		}
		this.awaitBest = null;
		const played = applyUci(this.chess, best.move);
		if (!played) {
			this.requestEngineMove();
			this.emit();
			return;
		}
		this.lastMove = { from: best.move.slice(0, 2), to: best.move.slice(2, 4) };
		this.clock.onMove(opponent(this.settings.playerColor));
		this.ponderMove = best.ponder ?? null;
		if (this.checkTerminal()) {
			this.emit();
			return;
		}
		this.startPonder();
		this.emit();
	}

	private onFlag(color: Color): void {
		if (this.over) {
			return;
		}
		this.haltEngine();
		const winner = opponent(color);
		if (!hasMatingMaterial(this.chess, winner)) {
			this.endGame({
				pgnResult: '1/2-1/2',
				reason: 'timeout',
				text: `${capitalize(color)} flagged, but insufficient material — draw`,
			});
			return;
		}
		this.endGame({
			pgnResult: winner === 'white' ? '1-0' : '0-1',
			reason: 'timeout',
			winner,
			text: `${capitalize(color)} ran out of time`,
		});
	}

	private checkTerminal(): boolean {
		if (this.chess.isCheckmate()) {
			const winner = opponent(turnColor(this.chess));
			this.haltEngine();
			this.endGame({
				pgnResult: winner === 'white' ? '1-0' : '0-1',
				reason: 'checkmate',
				winner,
				text: `${capitalize(winner)} wins by checkmate`,
			});
			return true;
		}
		if (this.chess.isStalemate()) {
			this.haltEngine();
			this.endGame({
				pgnResult: '1/2-1/2',
				reason: 'stalemate',
				text: 'Draw by stalemate',
			});
			return true;
		}
		if (this.chess.isThreefoldRepetition()) {
			this.haltEngine();
			this.endGame({
				pgnResult: '1/2-1/2',
				reason: 'threefold',
				text: 'Draw by threefold repetition',
			});
			return true;
		}
		if (this.chess.isInsufficientMaterial()) {
			this.haltEngine();
			this.endGame({
				pgnResult: '1/2-1/2',
				reason: 'insufficient',
				text: 'Draw by insufficient material',
			});
			return true;
		}
		if (this.chess.isDraw()) {
			this.haltEngine();
			this.endGame({
				pgnResult: '1/2-1/2',
				reason: 'draw',
				text: 'Draw',
			});
			return true;
		}
		return false;
	}

	private haltEngine(): void {
		if (this.pondering || this.awaitBest !== null) {
			this.pondering = false;
			this.ponderMove = null;
			this.awaitBest = 'discard';
			this.engine.stop();
		}
	}

	private endGame(result: GameResult): void {
		this.over = result;
		this.clock.stop();
		this.emit();
	}

	private emit(): void {
		const snap = this.snapshot();
		for (const listener of this.listeners) {
			listener(snap);
		}
	}
}

function hasMatingMaterial(chess: Chess, color: Color): boolean {
	const side = color === 'white' ? 'w' : 'b';
	let bishops = 0;
	let knights = 0;
	for (const row of chess.board()) {
		for (const piece of row) {
			if (!piece || piece.color !== side) {
				continue;
			}
			if (piece.type === 'q' || piece.type === 'r' || piece.type === 'p') {
				return true;
			}
			if (piece.type === 'b') {
				bishops += 1;
			}
			if (piece.type === 'n') {
				knights += 1;
			}
		}
	}
	return bishops >= 2 || (bishops >= 1 && knights >= 1);
}

function capitalize(color: Color): string {
	return color === 'white' ? 'White' : 'Black';
}
