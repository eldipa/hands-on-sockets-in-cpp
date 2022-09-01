.PHONY: all build tests next-commit prev-commit first-commit last-commit

all:

next-commit:
	@git checkout `git rev-list --topo-order HEAD..main | tail -1`

prev-commit:
	@git checkout HEAD~1

first-commit:
	@git checkout `git rev-list --max-parents=0 main | tail -1`

last-commit:
	@git checkout main
