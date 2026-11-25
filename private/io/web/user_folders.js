// The user folders live in memory, where reads and writes are synchronous. IDBFS loads them from the browser's IndexedDB
// before main and stores them back.
addToLibrary({
	$TgeUserFolders__deps: ['$FS', '$IDBFS', '$addRunDependency', '$removeRunDependency'],
	$TgeUserFolders: {
		root: '/home/web_user',
		isLoaded: false,
		isChanged: false,
		isStoring: false,
		problem: '',
		isReported: false,
		onStored: null,

		load: () => {
			if (TgeUserFolders.hasIndexedDB()) {
				FS.mount(IDBFS, { autoPersist: true }, TgeUserFolders.root);

				// IDBFS calls this for every change. Its own would store by itself and drop what goes wrong.
				IDBFS.queuePersist = () => { TgeUserFolders.isChanged = true; };

				addRunDependency('Tge user folders');

				FS.syncfs(true, (error) => {
					// After a failed load nothing is stored: that would delete from the store what never came out of it.
					TgeUserFolders.isLoaded = !error;
					TgeUserFolders.isChanged = false;
					TgeUserFolders.problem = error ? String(error) : '';

					removeRunDependency('Tge user folders');
				});
			} else {
				TgeUserFolders.problem = 'it has no IndexedDB for this page';
			}
		},

		hasIndexedDB: () => {
			try {
				return Boolean(globalThis.indexedDB);
			} catch (error) {
				// Some browsers refuse a page that may not use it the mere name.
				return false;
			}
		},

		store: () => {
			if (TgeUserFolders.isLoaded && TgeUserFolders.isChanged && !TgeUserFolders.isStoring) {
				TgeUserFolders.isChanged = false;
				TgeUserFolders.isStoring = true;

				FS.syncfs(false, (error) => {
					TgeUserFolders.isStoring = false;

					// A file that left its place while the store was reading is a change the next store takes.
					if (error && !(error instanceof FS.ErrnoError && TgeUserFolders.isChanged)) {
						TgeUserFolders.problem = String(error);
					}

					if (TgeUserFolders.onStored) {
						TgeUserFolders.finish();
					}
				});
			}
		},

		finish: () => {
			TgeUserFolders.store();

			if (!TgeUserFolders.isStoring) {
				TgeUserFolders.onStored();
			}
		},
	},

	// Only a program that asks for a user folder links this, and so loads them.
	TgeGetUserFoldersRoot__sig: 'p',
	TgeGetUserFoldersRoot__deps: ['$TgeUserFolders', '$addOnPreRun', '$stringToNewUTF8'],
	TgeGetUserFoldersRoot__postset: 'addOnPreRun(TgeUserFolders.load);',
	TgeGetUserFoldersRoot: () => stringToNewUTF8(TgeUserFolders.root),

	TgeStoreUserFolders__sig: 'v',
	TgeStoreUserFolders__deps: ['$TgeUserFolders'],
	TgeStoreUserFolders: () => TgeUserFolders.store(),

	TgeFinishUserFolders__sig: 'vp',
	TgeFinishUserFolders__deps: ['$TgeUserFolders', '$callUserCallback'],
	TgeFinishUserFolders: (onStored) => {
		{{{ runtimeKeepalivePush() }}}

		// From a later turn even when there is nothing to store: an exit inside main would not be the last word.
		TgeUserFolders.onStored = () => setTimeout(() => {
			{{{ runtimeKeepalivePop() }}}
			callUserCallback({{{ makeDynCall('v', 'onStored') }}});
		}, 0);

		TgeUserFolders.finish();
	},

	// Why nothing is stored, once; null when there is nothing to say.
	TgeTakeUserFoldersProblem__sig: 'p',
	TgeTakeUserFoldersProblem__deps: ['$TgeUserFolders', '$stringToNewUTF8'],
	TgeTakeUserFoldersProblem: () => {
		const isNews = TgeUserFolders.problem !== '' && !TgeUserFolders.isReported;

		TgeUserFolders.isReported ||= isNews;

		return isNews ? stringToNewUTF8(TgeUserFolders.problem) : {{{ to64('0') }}};
	},
});
