ENTRY(start)

SECTIONS {
	. = 1M;

	.boot :
	{
		/* puts header on top */
		*( .multiboot_header )
	}

	.text :
	{
		*( .text )
	}
}
