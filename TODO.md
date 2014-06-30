TODO
====

- [ ] Test new bound EVC
  - [x] Implement code in [getDatasetInfo.py](code/getDatasetInfo.py) to
    compute highest item frequency
  - [x] Implement code in [compareFIsVC.py](code/compareFIsVC.py) to use the
	highest item frequency for the bound
  - [x] Improve code to use highest item(set) frequency in the set that we care
	about
  - [x] Implement the code in [epsilon.py](code/epsilon.py) to use the
	highest item frequency for the bound
- [ ] Test new bound Rademacher
  - [x] Implement in [epsilon.py](code/epsilon.py) the code to compute the
	epsilon using other bounds to the shatter coefficient than the empirical
	VC-dimension
  - [ ] Implement code in [compareFIsVC.py](code/compareFIsVC.py) to use a
	different bound to the shatter coefficient
- [ ] Present new bound EVC
- [ ] Present 3-phases algorithm (test first)
- [ ] Check [SDM reviews](paper/SDM14/ReviewsSDM.pdf) for other changes
- [ ] The LaTeX sources contain a lot of comments. Check to see if anything is
  relevant
- [ ] General pass

