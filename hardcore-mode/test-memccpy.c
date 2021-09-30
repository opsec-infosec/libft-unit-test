#include "utils.h"

typedef void *(*proto_t) (void *, const void *, int c, size_t);

	static void
do_one_test (impl_t *impl, void *dst, const void *src, int c, size_t len,
	    size_t n)
{
  	void *expect = len > n ? NULL : (char *) dst + len;
  	if (CALL (impl, dst, src, c, n) != expect)
    {
      	error (0, 0, "Wrong result in function %s %p %p", impl->name,
	     		CALL (impl, dst, src, c, n), expect);
      	ret = 1;
      	return;
    }

  	if (memcmp (dst, src, len > n ? n : len) != 0)
    {
      	error (0, 0, "Wrong result in function %s", impl->name);
      	ret = 1;
      	return;
    }
}

	static void
do_test (size_t align1, size_t align2, int c, size_t len, size_t n,
	 	int max_char)
{
  	size_t i;
  	char *s1, *s2;

  	align1 &= 7;
  	if (align1 + len >= page_size)
    	return;

  	align2 &= 7;
  	if (align2 + len >= page_size)
    	return;

  	s1 = (char *) (buf1 + align1);
  	s2 = (char *) (buf2 + align2);

  	for (i = 0; i < len - 1; ++i)
    {
      	s1[i] = 32 + 23 * i % (max_char - 32);
      	if (s1[i] == (char) c)
			--s1[i];
    }
  	s1[len - 1] = c;
  	for (i = len; i + align1 < page_size && i < len + 64; ++i)
    	s1[i] = 32 + 32 * i % (max_char - 32);

  	FOR_EACH_IMPL (impl, 0)
    	do_one_test (impl, s2, s1, c, len, n);
}

	static void
do_random_tests (void)
{
  	size_t i, j, n, align1, align2, len, size, mode;
  	unsigned char *p1 = buf1 + page_size - 512;
  	unsigned char *p2 = buf2 + page_size - 512;
  	unsigned char *res, c;

  	for (n = 0; n < ITERATIONS; n++)
    {
      	mode = random ();
      	c = random ();
      	if (mode & 1)
		{
	  		size = random () & 255;
	  		align1 = 512 - size - (random () & 15);
	  		if (mode & 2)
	    		align2 = align1 - (random () & 24);
	  		else
	    		align2 = align1 - (random () & 31);
	  		if (mode & 4)
	    	{
	      		j = align1;
	      		align1 = align2;
	      		align2 = j;
	    	}
	  		if (mode & 8)
	    		len = size - (random () & 31);
	  		else
	    		len = 512;
	  		if (len >= 512)
	    		len = random () & 511;
		}
      	else
		{
	  		align1 = random () & 31;
	  		if (mode & 2)
	    		align2 = random () & 31;
	  		else
	    		align2 = align1 + (random () & 24);
	  		len = random () & 511;
	  		j = align1;
	  		if (align2 > j)
	    		j = align2;
	  		if (mode & 4)
	    	{
	      		size = random () & 511;
	      		if (size + j > 512)
					size = 512 - j - (random() & 31);
	    	}
	  		else
	    		size = 512 - j;
	  		if ((mode & 8) && len + j >= 512)
	    		len = 512 - j - (random () & 7);
		}
      	j = len + align1 + 64;
      	if (j > 512)
			j = 512;
      	for (i = 0; i < j; i++)
		{
	  		if (i == len + align1)
	    		p1[i] = c;
	  		else
	    	{
	      		p1[i] = random () & 255;
	      		if (i >= align1 && i < len + align1 && p1[i] == c)
					p1[i] = (random () & 127) + 3 + c;
	    	}
		}

      	FOR_EACH_IMPL (impl, 1)
		{
	  		unsigned char *expect;
	  		memset (p2 - 64, '\1', 512 + 64);
	  		res = CALL (impl, p2 + align2, p1 + align1, (char) c, size);
	  		if (len >= size)
	    		expect = NULL;
	  		else
	    		expect = p2 + align2 + len + 1;

	  		if (res != expect)
	    	{
	      		error (0, 0, "Iteration %zd - wrong result in function %s (%zd, %zd, %zd, %zd, %d) %p != %p",
		     			n, impl->name, align1, align2, len, size, c, res, expect);
	      		ret = 1;
	    	}
	  		for (j = 0; j < align2 + 64; ++j)
	    	{
	      		if (p2[j - 64] != '\1')
				{
		  			error (0, 0, "Iteration %zd - garbage before, %s (%zd, %zd, %zd)",
			 				n, impl->name, align1, align2, len);
		  			ret = 1;
		  			break;
				}
	    	}
	  		j = align2 + len + 1;
	  		if (size + align2 < j)
	    		j = size + align2;
	  		for (; j < 512; ++j)
	    	{
	      		if (p2[j] != '\1')
				{
		  			error (0, 0, "Iteration %zd - garbage after, %s (%zd, %zd, %zd)",
			 				n, impl->name, align1, align2, len);
		  			ret = 1;
		  			break;
				}
	    	}
	  		j = len + 1;
	  		if (size < j)
	    		j = size;
	  		if (memcmp (p1 + align1, p2 + align2, j))
	    	{
	      		error (0, 0, "Iteration %zd - different strings, %s (%zd, %zd, %zd)",
		     			n, impl->name, align1, align2, len);
	      		ret = 1;
	    	}
		}
    }
}

static void
