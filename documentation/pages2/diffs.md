
\page diffs Diffs

Diff communication is the lifeblood of the system.  Here's how it works:

 - Data is for the most part JSON (there are exceptions)
 - JSON objects, including diffs, are zipped together recursively.  Colliding names are clobbered from source to destination.
 - null is magic; it removes attributes.

For example:

    MLB1:
    {
        "name": "Joe DiMaggio",
        "average": .325,
        "metadata": {
                 "skill": "baseball",
                 "girlfriend": "Marilyn Monroe"
             }
    }

    MLB2:
    {
        "name": "A-Rod",
        "metadata": {
                 "skill": "football",
                 "girlfriend": "Madonna"
            }
    }

    DIFF:
    {
        "average": null,
        "metadata": {
                 "skill": null,
                 "girlfriend": null
            }
    }

If these were diffed against each other conceptually:

    MLB = applyDiff(MLB1,MLB2):         //base, then diff

    {
        "name": "A-Rod",
        "average": .325,                //which would be in his dreams
        "metadata": {
                 "skill": "football",
                 "girlfriend": "Madonna"
            }
    }

    but later:

    MLB = applyDiff(MLB,DIFF):          //base, then diff

    {
        "name": "A-Rod"
        "metadata": {
            }
    }

    So, no skill, no average, and no girlfriend.

FIXME: provide a better example with more edge cases.

Arrays are treated as entries, and are not intersected.  They clobber each other.

A future incarnation will adopt BSON instead of JSON.  Perhaps Qt will make this adoption, or we will have to incorporate it some other way.

