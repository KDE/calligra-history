main
{
	println("1------");
	r = QRect();
	println("2------");
	r.left = 100;
	r.right = 156;
	r.top = 20;
	r.bottom = 40;
	println( r.width, r.height );
	println("3------");
	println( r.left );
	println("4------");
	r2 = r;
	println("5------");
	r2.left = 55;
	println("6------");
	println( r.left, r2.left );
	println("7------");
	r3 = QRect();
	println( r.isNull(), r3.isNull() );
	println("8------");
	r2.left = 10;
	r2.right = 256;
	r2.top = 10;
	r2.bottom = 50;	
	println( r.contains( r2 ), r2.contains( r ) );
	println("9------");
	r2.left = 300;
	println("9a------");
	println( r2.normalize() );
	println("9b------");
	r4 = r2.normalize();
	println("9c------");
	println( r2 );
	println("10------");
	println( r4 );
	println( r4.left, r4.right );
	println("11------");
	p = QPoint();
	println("12------");
	p2 = QPoint( 5, 6 );
	println("13------");
	println( p.isNull(), p2.isNull() );
	println("14------");
	println( p, p2 );
	println("15------");
	r5 = QRect( 1, 2, 6, 4 );
	r6 = QRect( QPoint( 3,4 ), QPoint( 5, 9 ) );
	r7 = QRect( QPoint( 44, 55 ), QSize( 100, 200 ) );
	println( r5, r6, r7 );
	println("16------");
	r7.size = QSize( 122, 133 );
	println( r7 );
	println("17------");
	println( r7.contains( QRect( 50, 60, 10, 10 ) ) );
	println( r7.contains( QPoint( 44, 55 ) ) );
	println( r7.contains( QRect( 50, 60, 10, 10 ), TRUE ) );
	println( r7.contains( QPoint( 44, 55 ), TRUE ) );	
}
