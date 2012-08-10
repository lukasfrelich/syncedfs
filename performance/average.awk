BEGIN{
	l=2;
	c=0;
	nf=0;
}
c < l {
	for (i = 1; i <= NF; i++) {
		a[i] = a[i]+$i;
	}
	c=c+1;
	if ( nf < NF ) {
		nf = NF;
	}
}
c == l {
	for (i = 1; i <= nf; i++) {
		printf("%s ", a[i]/l);
		a[i] = 0;
	}
	printf("\n");
	c=0;
	nf=0;
}
END{
	if ( c > 0 ) {
		for (i = 1; i <= nf; i++) {
			printf("%s", a[i]/l);
		}
		printf("\n");
	}
}
