{
	'targets': [
		{
			'target_name': 'gcontext',
			'sources': [
				'src/node-gcontext.cpp',
				'src/gcontext.cpp'
			],
			'include_dirs': [
				"<!(node -e \"require('nan')\")"
			],
			'conditions': [
				['OS=="linux"', {
					'defines': [
						'LIB_EXPAT=expat'
					],
					'cflags': [
						'<!@(pkg-config --cflags glib-2.0)'
					],
					'ldflags': [
						'<!@(pkg-config  --libs-only-L --libs-only-other glib-2.0)'
					],
					'libraries': [
						'<!@(pkg-config  --libs-only-l --libs-only-other glib-2.0)'
					]
				}],
				['OS=="mac"', {
					'include_dirs': [
						'<!@(tools/mac-config.sh --include-dirs)'
					]
				}]
			]
		}
	]
}
