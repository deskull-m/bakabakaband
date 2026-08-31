## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

visual_studio_files = \
	VisualStudio/.gitattributes \
	VisualStudio/Bakabakaband.sln \
	VisualStudio/Bakabakaband/Bakabakaband.Common.props \
	VisualStudio/Bakabakaband/Bakabakaband.Configuration.props \
	VisualStudio/Bakabakaband/Bakabakaband.vcxproj \
	VisualStudio/Bakabakaband/Bakabakaband.vcxproj.filters \
	VisualStudio/Bakabakaband/packages.config \
	VisualStudio/Hengband/Hengband.vcxproj \
	VisualStudio/Hengband/Hengband.vcxproj.filters

schema_files = \
	schema/ArtifactDefinitions.schema.json \
	schema/BaseitemDefinitions.schema.json \
	schema/ClassMagicDefinitions.schema.json \
	schema/CreatureParty.schema.json \
	schema/DungeonDefinitions.schema.json \
	schema/MonraceDefinitions.schema.json \
	schema/MonsterMessages.schema.json \
	schema/Quest.schema.json \
	schema/QuestPreferences.schema.json \
	schema/SpellDefinitions.schema.json \
	schema/TerrainDefinitions.schema.json

EXTRA_DIST = \
	autopick.txt \
	autopick_eng.txt \
	bootstrap \
	readme.md \
	readme_angband \
	readme-eng.md \
	THIRD-PARTY-NOTICES.txt \
	hengband.spec \
	tools/bot/hbctl.py \
	tools/bot/README.md \
	$(visual_studio_files) \
	$(schema_files)

SUBDIRS = src lib
