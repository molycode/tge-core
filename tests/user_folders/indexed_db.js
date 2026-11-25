// What IDBFS uses of an IndexedDB, for Node: the records live in the file TGE_TEST_STORE names, which a transaction
// replaces as a whole when it completes. Without the variable there is none, and TGE_TEST_STORE_FAULT names the call that
// fails: open, get or put. A worker runs this script too, and has no business with the file.
if (process.env.TGE_TEST_STORE && require('node:worker_threads').isMainThread) {
	const fs = require('node:fs');
	const v8 = require('node:v8');
	const path = process.env.TGE_TEST_STORE;

	let records = fs.existsSync(path) ? v8.deserialize(fs.readFileSync(path)) : new Map();

	const failIf = (call, name) => {
		if (process.env.TGE_TEST_STORE_FAULT === call) {
			throw new DOMException(`the store fails to ${call}`, name);
		}
	};

	const beginTransaction = (names, mode) => {
		const draft = new Map(records);
		const transaction = {};
		let numPending = 0;

		const later = (action) => {
			++numPending;

			setImmediate(() => {
				action();

				if (--numPending === 0) {
					if (mode === 'readwrite') {
						records = draft;
						fs.writeFileSync(path, v8.serialize(records));
					}

					transaction.oncomplete?.({ target: transaction });
				}
			});
		};

		const ask = (answer) => {
			const request = {};

			later(() => {
				try {
					request.result = answer();
				} catch (error) {
					request.error = error;
				}

				(request.error ? request.onerror : request.onsuccess)?.({ target: request, preventDefault: () => {} });
			});

			return request;
		};

		const openKeyCursor = () => {
			const keys = [...draft.keys()];
			const request = {};

			const step = (index) => later(() => request.onsuccess({ target: { result: (index < keys.length)
				? { primaryKey: keys[index], key: draft.get(keys[index]).timestamp, continue: () => step(index + 1) } : null } }));

			step(0);

			return request;
		};

		transaction.objectStore = () => ({
			get: (key) => ask(() => (failIf('get', 'UnknownError'), draft.get(key))),
			put: (value, key) => {
				const copy = structuredClone(value);

				return ask(() => (failIf('put', 'QuotaExceededError'), draft.set(key, copy)));
			},
			delete: (key) => ask(() => draft.delete(key)),
			index: () => ({ openKeyCursor }),
		});

		return transaction;
	};

	globalThis.indexedDB = {
		open: () => {
			failIf('open', 'SecurityError');

			const request = {};

			setImmediate(() => {
				request.result = { transaction: beginTransaction, close: () => {} };
				request.onsuccess({ target: request });
			});

			return request;
		},
	};
}
