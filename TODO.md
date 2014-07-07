TODO
====

- [x] Test new bound EVC
  - [x] Implement code in [getDatasetInfo.py](code/getDatasetInfo.py) to
    compute highest item frequency
  - [x] Implement code in [compareFIsVC.py](code/compareFIsVC.py) to use the
	highest item frequency for the bound
  - [x] Improve code to use highest item(set) frequency in the set that we care
	about
  - [x] Implement the code in [epsilon.py](code/epsilon.py) to use the
	highest item frequency for the bound
- [x] Test new bound Rademacher
  - [x] Implement in [epsilon.py](code/epsilon.py) the code to compute the
	epsilon using other bounds to the shatter coefficient than the empirical
	VC-dimension
  - [x] Implement code in [compareFIsVC.py](code/compareFIsVC.py) to use a
	different bound to the shatter coefficient
- [x] Check code binomial
- [x] Check code holdout
- [x] Write glue code for experiments
- [x] Write code holdoutVC
  - [x] write python code
  - [x] write glue code
- [ ] Present holdoutVC
- [ ] Present new bound EVC
- [ ] Check [SDM reviews](paper/SDM14/ReviewsSDM.pdf) for other changes
- [ ] The LaTeX sources contain a lot of comments. Check to see if anything is
  relevant
- [ ] General pass

