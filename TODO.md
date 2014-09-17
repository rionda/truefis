TODO
====

The order of the tasks reflects the order in which I plan to do them.

- [ ] Present new refined bound that uses information about the variance, using
	Thm 3.3 from BucheronBL05 survey + the variance bound presented in
	BoucheronBLXX (Concentration inequalities). There's a mail from Matteo from
	July 16 that explains how to do that. We should have the proof in full
	because 1) it is not presented anywhere; 2) looks good to reviewers; 3) can
	generate citations ;). This goes in Sect. 3.2. Expected Needed Time: 2-3
	hours.
- [ ] Change the presentation of the VC-non-holdout method to use the refined
	bound (above) to compute the second epsilon. Expected Needed Time: 1-2
	hours.
- [ ] Change the code of the VC-non-holdout method to use the refined bound.
	Expected Needed Time: 2-3 hours.
- [ ] Run experiments with new VC-non-holdout code. Expected Needed Time: 1
	night.
- [ ] Collect experiment results for the new VC-non holdout and present them in
	Sect. 6. Expected Time Needed: 4-5 hours.
- [ ] Present holdoutVC in the paper. By holdoutVC I mean the algorithm that
	uses the holdout approach combined with the VC-dimension: we split the
	dataset into two parts, use one to extract a set of candidates (using VC
	bounds + SUKP). This goes in a new subsection of Sect.5.
	This may require a huge rewrite. Expected Needed Time: 8-10 hours.
- [ ] Write new code holdoutVC. Implement the refined sample size bound.
	Expected Needed Time: 2-3 hours.
- [ ] Run experiments with new holdoutVC code. Expected Needed Time: 1 night.
- [ ] Collect experiment results of holdoutVC and write experiment reports.
	Expected Needed Time: 6 hours.
- [ ] Present holdout results in paper. Expected Needed Time: 3-4 hours.
- [ ] Check [SDM reviews](paper/SDM14/ReviewsSDM.pdf) for other changes.
	Expected Needed Time: 2-3 hours.
- [ ] The LaTeX sources contain a lot of comments. Check to see if anything is
  relevant. Expected Needed Time: 1-2 hours to go through comments + additional
  work if needed.
- [ ] Various passes to check organization and clarity of material. Expected
	Needed Time: 1-2 hours per pass.
- [ ] Make sure code is ready for publication. Expected Needed time: 2-3 hours.
- [ ] General pass before submission. Expected Needed Time: 2-3 hours.

