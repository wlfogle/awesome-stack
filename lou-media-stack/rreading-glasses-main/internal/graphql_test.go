package internal

import (
	"context"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"
	"sync"
	"sync/atomic"
	"testing"
	"time"

	"github.com/blampe/rreading-glasses/gr"
	"github.com/blampe/rreading-glasses/hardcover"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"github.com/vektah/gqlparser/v2/gqlerror"
)

func TestQueryBuilderMultipleQueries(t *testing.T) {
	t.Run("hardcover", func(t *testing.T) {
		qb := newQueryBuilder()

		query1 := hardcover.GetBook_Operation
		vars1 := map[string]interface{}{"grBookIDs": []string{"1"}}

		query2 := hardcover.GetAuthorEditions_Operation
		vars2 := map[string]any{
			"id":     1,
			"limit":  2,
			"offset": 3,
		}

		id1, _, err := qb.add(query1, vars1)
		require.NoError(t, err)

		id2, _, err := qb.add(query2, vars2)
		require.NoError(t, err)

		query, vars, err := qb.build()
		require.NoError(t, err)

		expected := fmt.Sprintf(`query GetBook($%s_grBookID: String!, $%s_id: Int!, $%s_limit: Int!, $%s_offset: Int!) {
  %s: book_mappings(limit: 1, where: {platform_id: {_eq: 1}, external_id: {_eq: $%s_grBookID}}) {
    external_id
    edition {
      id
      title
      subtitle
      asin
      isbn_13
      edition_format
      pages
      audio_seconds
      language {
        language
      }
      publisher {
        name
      }
      release_date
      description
      identifiers
      book_id
    }
    book {
      id
      title
      subtitle
      description
      release_date
      cached_tags(path: "$.Genre")
      cached_image(path: "url")
      contributions {
        contributable_type
        contribution
        author {
          id
          name
          slug
          bio
          cached_image(path: "url")
        }
      }
      slug
      book_series {
        position
        series {
          id
          name
          description
          identifiers
        }
      }
      book_mappings {
        dto_external
      }
      rating
      ratings_count
    }
  }
  %s: authors(limit: 1, where: {id: {_eq: $%s_id}}) {
    location
    id
    slug
    contributions(limit: $%s_limit, offset: $%s_offset, order_by: {id: asc}, where: {contributable_type: {_eq: "Book"}}) {
      book {
        id
        title
        ratings_count
        book_mappings(limit: 1, where: {platform_id: {_eq: 1}}) {
          book_id
          edition_id
          external_id
        }
      }
    }
    identifiers(path: "goodreads[0]")
  }
}`, id1, id2, id2, id2, id1, id1, id2, id2, id2, id2)

		assert.Equal(t, expected, query)

		assert.Len(t, vars, 4)
		assert.Contains(t, vars, id1+"_grBookID", id2+"_id", id2+"_limit", id2+"_offset")
	})

	t.Run("gr", func(t *testing.T) {
		qb := newQueryBuilder()

		query1 := gr.GetBook_Operation
		vars1 := map[string]interface{}{"legacyId": []string{"1"}}

		query2 := gr.GetAuthorWorks_Operation
		vars2 := map[string]any{
			"pagination":                 map[string]string{},
			"getWorksByContributorInput": map[string]string{},
		}

		id1, _, err := qb.add(query1, vars1)
		require.NoError(t, err)

		id2, _, err := qb.add(query2, vars2)
		require.NoError(t, err)

		query, vars, err := qb.build()
		require.NoError(t, err)

		expected := fmt.Sprintf(`query GetBook($%s_legacyId: Int!, $%s_getWorksByContributorInput: GetWorksByContributorInput!, $%s_pagination: PaginationInput!) {
  %s: getBookByLegacyId(legacyId: $%s_legacyId) {
    ...BookInfo
    work {
      id
      legacyId
      details {
        webUrl
        publicationTime
      }
      bestBook {
        legacyId
        title
        titlePrimary
      }
      editions {
        edges {
          node {
            ...BookInfo
          }
        }
      }
    }
  }
  %s: getWorksByContributor(getWorksByContributorInput: $%s_getWorksByContributorInput, pagination: $%s_pagination) {
    edges {
      node {
        id
        bestBook {
          legacyId
          primaryContributorEdge {
            role
            node {
              legacyId
            }
          }
          secondaryContributorEdges {
            role
          }
        }
      }
    }
    pageInfo {
      hasNextPage
      nextPageToken
    }
  }
}
fragment BookInfo on Book {
  id
  legacyId
  description(stripped: true)
  bookGenres {
    genre {
      name
    }
  }
  bookSeries {
    series {
      id
      title
      webUrl
    }
    seriesPlacement
  }
  details {
    asin
    isbn13
    format
    numPages
    language {
      name
    }
    officialUrl
    publisher
    publicationTime
  }
  imageUrl
  primaryContributorEdge {
    node {
      id
      name
      legacyId
      webUrl
      profileImageUrl
      description
    }
  }
  stats {
    averageRating
    ratingsCount
    ratingsSum
  }
  title
  titlePrimary
  webUrl
}`, id1, id2, id2, id1, id1, id2, id2, id2)

		assert.Equal(t, expected, query)

		assert.Len(t, vars, 3)
		assert.Contains(t, vars, id1+"_legacyId", id2+"_getWorksByContributorInput", id2+"_pagination")
	})
}

func TestBatching(t *testing.T) {
	apiKey := os.Getenv("HARDCOVER_API_KEY")
	if apiKey == "" {
		t.Skip("missing HARDCOVER_API_KEY")
		return
	}
	transport := &HeaderTransport{
		Key:          "Authorization",
		Value:        "Bearer " + apiKey,
		RoundTripper: http.DefaultTransport,
	}

	client := &http.Client{Transport: transport}

	url := "https://api.hardcover.app/v1/graphql"

	gql, err := NewBatchedGraphQLClient(url, client, time.Second, 6)
	require.NoError(t, err)

	start := time.Now()

	wg := sync.WaitGroup{}
	wg.Add(1)
	go func() {
		defer wg.Done()
		_, err := hardcover.GetBook(context.Background(), gql, "0156028352")
		if err != nil {
			panic(err)
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		_, err := hardcover.GetBook(context.Background(), gql, "0164005178")
		if err != nil {
			panic(err)
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		_, err := hardcover.GetBook(context.Background(), gql, "0340640138")
		if err != nil {
			panic(err)
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		_, err := hardcover.GetBook(context.Background(), gql, "missing")
		if err != nil {
			panic(err)
		}
	}()

	wg.Wait()

	assert.Less(t, time.Since(start), 4*time.Second)
}

func TestBatchingOverflow(t *testing.T) {
	calls := atomic.Int32{}

	client := &http.Client{
		Transport: roundTripperFunc(func(r *http.Request) (*http.Response, error) {
			calls.Add(1)
			body := `{"data": {}, "errors": []}`
			return &http.Response{
				StatusCode: 200,
				Body:       io.NopCloser(strings.NewReader(body)),
			}, nil
		}),
	}

	gql, err := NewBatchedGraphQLClient("https://foo.com", client, 50*time.Millisecond, 1)
	require.NoError(t, err)

	wg := sync.WaitGroup{}

	// var resp1, resp2 *gr.GetBookResponse
	var err1, err2 error

	// Spawn more queries than our batch allows. They should get executed in
	// separate batches.
	wg.Add(2)
	go func() {
		defer wg.Done()
		_, err1 = gr.GetBook(t.Context(), gql, 1)
	}()
	go func() {
		defer wg.Done()
		_, err2 = gr.GetBook(t.Context(), gql, 2)
	}()
	wg.Wait()

	assert.NoError(t, err1)
	assert.NoError(t, err2)

	assert.Equal(t, int32(2), calls.Load())
}

type roundTripperFunc func(*http.Request) (*http.Response, error)

func (fn roundTripperFunc) RoundTrip(r *http.Request) (*http.Response, error) {
	return fn(r)
}

func TestGQLStatusCode(t *testing.T) {
	err := &gqlerror.Error{Message: "womp"}
	assert.ErrorIs(t, err, gqlStatusErr(err))

	err = &gqlerror.Error{Message: "Request failed with status code 403"}
	err403 := statusErr(403)
	assert.ErrorAs(t, gqlStatusErr(err), &err403)
}
