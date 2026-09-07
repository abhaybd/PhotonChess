import type { Chess } from 'chess.js';
import { timeControlTag, type GameResult, type GameSettings } from './types';

const TERMINATION: Record<GameResult['reason'], string> = {
	checkmate: 'checkmate',
	stalemate: 'stalemate',
	draw: 'draw',
	threefold: 'threefold repetition',
	insufficient: 'insufficient material',
	timeout: 'time forfeit',
	resign: 'abandoned',
};

export function buildPgn(chess: Chess, settings: GameSettings, result?: GameResult | null): string {
	const now = new Date();
	const yyyy = now.getFullYear();
	const mm = String(now.getMonth() + 1).padStart(2, '0');
	const dd = String(now.getDate()).padStart(2, '0');
	const white = settings.playerColor === 'white' ? 'You' : 'Photon';
	const black = settings.playerColor === 'black' ? 'You' : 'Photon';
	const pgnResult = result?.pgnResult ?? '*';

	chess.setHeader('Event', 'Photon Chess');
	chess.setHeader('Site', 'Photon Web');
	chess.setHeader('Date', `${yyyy}.${mm}.${dd}`);
	chess.setHeader('White', white);
	chess.setHeader('Black', black);
	chess.setHeader('Result', pgnResult);
	chess.setHeader('TimeControl', timeControlTag(settings));
	if (result) {
		chess.setHeader('Termination', TERMINATION[result.reason]);
	} else {
		chess.removeHeader('Termination');
	}

	let pgn = chess.pgn();
	if (!pgn.endsWith(pgnResult)) {
		pgn = `${pgn} ${pgnResult}`;
	}
	return pgn.trim() + '\n';
}
