import 'chessground/assets/chessground.base.css';
import 'chessground/assets/chessground.brown.css';
import 'chessground/assets/chessground.cburnett.css';
import './style.css';
import { BoardView } from './board';
import { formatAbsoluteScore, PhotonEngine } from './engine';
import { GameController } from './game';
import { buildPgn } from './pgn';
import {
	formatClock,
	resolveSide,
	TIME_PRESETS,
	type GameSettings,
	type SideChoice,
} from './types';

const loadingScreen = el<HTMLElement>('screen-loading');
const loadingText = el<HTMLElement>('loading-text');
const setupScreen = el<HTMLElement>('screen-setup');
const playScreen = el<HTMLElement>('screen-play');
const boardEl = el<HTMLElement>('board');
const promoEl = el<HTMLElement>('promo-overlay');
const clockTop = el<HTMLElement>('clock-top');
const clockBottom = el<HTMLElement>('clock-bottom');
const moveList = el<HTMLOListElement>('move-list');
const engineStatus = el<HTMLElement>('engine-status');
const resultDialog = el<HTMLDialogElement>('result-dialog');
const resultTitle = el<HTMLElement>('result-title');
const resultDetail = el<HTMLElement>('result-detail');
const resultPgn = el<HTMLElement>('result-pgn');
const customTime = el<HTMLElement>('custom-time');
const copyPgnBtn = el<HTMLButtonElement>('btn-export-pgn');
const copyMovesPgnBtn = el<HTMLButtonElement>('btn-copy-moves-pgn');
const resignBtn = el<HTMLButtonElement>('btn-resign');
const boardWrap = el<HTMLElement>('board-wrap');

let engine: PhotonEngine | null = null;
let game: GameController | null = null;
let board: BoardView | null = null;
let unsubGame: (() => void) | null = null;
let resultShown = false;

function el<T extends HTMLElement>(id: string): T {
	const node = document.getElementById(id);
	if (!node) {
		throw new Error(`Missing #${id}`);
	}
	return node as T;
}

function selectedValue(name: string): string {
	const input = document.querySelector<HTMLInputElement>(`input[name="${name}"]:checked`);
	if (!input) {
		throw new Error(`No selection for ${name}`);
	}
	return input.value;
}

function readSettings(): GameSettings {
	const side = resolveSide(selectedValue('side') as SideChoice);
	const preset = selectedValue('time');
	if (preset === 'custom') {
		const minutes = Number(el<HTMLInputElement>('custom-min').value);
		const increment = Number(el<HTMLInputElement>('custom-inc').value);
		if (!Number.isFinite(minutes) || minutes <= 0) {
			throw new Error('Enter a positive time in minutes.');
		}
		if (!Number.isFinite(increment) || increment < 0) {
			throw new Error('Increment cannot be negative.');
		}
		return {
			playerColor: side,
			initialMs: minutes * 60_000,
			incrementMs: increment * 1_000,
		};
	}
	const times = TIME_PRESETS[preset];
	if (!times) {
		throw new Error('Unknown time control');
	}
	return { playerColor: side, ...times };
}

function show(screen: HTMLElement): void {
	loadingScreen.hidden = screen !== loadingScreen;
	setupScreen.hidden = screen !== setupScreen;
	playScreen.hidden = screen !== playScreen;
}

function setClock(node: HTMLElement, ms: number, active: boolean, name: string): void {
	node.classList.toggle('active', active);
	node.classList.toggle('low', ms < 10_000);
	node.querySelector('.clock-label')!.textContent = name;
	node.querySelector('.clock-time')!.textContent = formatClock(ms);
}

function renderMoves(sans: string[]): void {
	const rows: string[] = [];
	for (let i = 0; i < sans.length; i += 2) {
		const n = i / 2 + 1;
		const white = sans[i] ?? '';
		const black = sans[i + 1] ?? '';
		const whiteClass = i === sans.length - 1 ? ' current' : '';
		const blackClass = i + 1 === sans.length - 1 ? ' current' : '';
		rows.push(
			`<li><span class="move-num">${n}.</span>` +
				`<span class="san${whiteClass}">${white}</span>` +
				`<span class="san${blackClass}">${black}</span></li>`,
		);
	}
	moveList.innerHTML = rows.join('');
	moveList.scrollTop = moveList.scrollHeight;
}

function engineLine(snap: ReturnType<GameController['snapshot']>): string {
	if (snap.over) {
		return snap.over.text;
	}
	if (snap.thinking) {
		const info = snap.engineInfo;
		if (info?.depth && info.score) {
			return `Photon thinking  depth ${info.depth} (${formatAbsoluteScore(info.score, snap.turn)})`;
		}
		return 'Photon thinking…';
	}
	if (snap.movable) {
		return 'Your move';
	}
	return 'Photon thinking…';
}

function startGame(): void {
	if (!engine) {
		return;
	}
	let settings: GameSettings;
	try {
		settings = readSettings();
	} catch (error) {
		window.alert(error instanceof Error ? error.message : String(error));
		return;
	}

	unsubGame?.();
	game?.dispose();
	game = new GameController(engine, settings);

	if (!board) {
		board = new BoardView(
			boardEl,
			promoEl,
			(from, to, promotion) => {
				game?.playPlayerMove(from, to, promotion);
			},
			(from, to) => game?.needsPromotion(from, to) ?? false,
		);
	}

	const player = settings.playerColor;
	const opp = player === 'white' ? 'black' : 'white';
	clockBottom.dataset.color = player;
	clockTop.dataset.color = opp;
	resultShown = false;
	applyResultChrome(null, player);
	promoEl.querySelector('.promo-choices')?.setAttribute('data-color', player);
	promoEl.querySelectorAll('piece').forEach((piece) => {
		piece.classList.remove('white', 'black');
		piece.classList.add(player);
	});

	unsubGame = game.subscribe((snap) => {
		board?.apply(snap);
		const playerTurn = snap.turn === player;
		setClock(
			clockBottom,
			snap.clocks[player],
			!snap.over && playerTurn,
			player === 'white' ? 'You · White' : 'You · Black',
		);
		setClock(
			clockTop,
			snap.clocks[opp],
			!snap.over && !playerTurn,
			opp === 'white' ? 'Photon · White' : 'Photon · Black',
		);
		engineStatus.textContent = engineLine(snap);
		renderMoves(snap.sans);
		applyResultChrome(snap.over, player);
		if (snap.over && !resultShown) {
			resultShown = true;
			resultTitle.textContent = snap.over.text;
			resultDetail.textContent =
				snap.over.pgnResult === '1/2-1/2'
					? '½–½'
					: snap.over.winner === 'white'
						? '1–0'
						: '0–1';
			resultPgn.textContent = buildPgn(game!.chess, game!.settings, snap.over);
			copyPgnBtn.textContent = 'Copy PGN to clipboard';
			resultDialog.showModal();
		}
	});

	show(playScreen);
	game.start();
	requestAnimationFrame(() => board?.resize());
}

function playAgain(): void {
	resultDialog.close();
	unsubGame?.();
	unsubGame = null;
	game?.dispose();
	game = null;
	show(setupScreen);
}

function currentPgn(): string | null {
	if (!game) {
		return null;
	}
	return buildPgn(game.chess, game.settings, game.snapshot().over);
}

async function copyPgn(button: HTMLButtonElement, copiedLabel = 'Copied!'): Promise<void> {
	const pgn = currentPgn();
	if (!pgn) {
		return;
	}
	const previous = button.textContent;
	try {
		await navigator.clipboard.writeText(pgn);
		button.textContent = copiedLabel;
		window.setTimeout(() => {
			button.textContent = previous;
		}, 1500);
	} catch {
		window.prompt('Copy PGN:', pgn);
	}
}

function applyResultChrome(
	over: ReturnType<GameController['snapshot']>['over'],
	playerColor: GameSettings['playerColor'],
): void {
	boardWrap.classList.remove('result-win', 'result-loss', 'result-draw');
	if (!over) {
		resignBtn.textContent = 'Resign';
		resignBtn.classList.add('danger');
		resignBtn.classList.remove('primary');
		return;
	}
	resignBtn.textContent = 'New game';
	resignBtn.classList.remove('danger');
	resignBtn.classList.add('primary');
	if (over.pgnResult === '1/2-1/2') {
		boardWrap.classList.add('result-draw');
	} else if (over.winner === playerColor) {
		boardWrap.classList.add('result-win');
	} else {
		boardWrap.classList.add('result-loss');
	}
}

el<HTMLButtonElement>('btn-start').addEventListener('click', startGame);
resignBtn.addEventListener('click', () => {
	if (game?.snapshot().over) {
		playAgain();
		return;
	}
	game?.resign();
});
el<HTMLButtonElement>('btn-view-board').addEventListener('click', () => {
	resultDialog.close();
});
resultDialog.addEventListener('click', (event) => {
	if (event.target === resultDialog) {
		resultDialog.close();
	}
});
copyPgnBtn.addEventListener('click', () => {
	void copyPgn(copyPgnBtn);
});
copyMovesPgnBtn.addEventListener('click', () => {
	void copyPgn(copyMovesPgnBtn);
});

document.querySelectorAll<HTMLInputElement>('input[name="time"]').forEach((input) => {
	input.addEventListener('change', () => {
		customTime.hidden = selectedValue('time') !== 'custom';
	});
});

window.addEventListener('resize', () => board?.resize());

async function boot(): Promise<void> {
	try {
		engine = await PhotonEngine.load();
		show(setupScreen);
	} catch (error) {
		console.error(error);
		loadingText.textContent =
			'Could not load Photon. Build the WASM engine first (./scripts/build-web.sh), then reload.';
		loadingScreen.classList.add('error');
	}
}

void boot();
