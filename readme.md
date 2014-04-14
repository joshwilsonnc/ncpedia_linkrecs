<h5>I recently discovered a few problematic things with this module and need to
address them. Planning to fix April 2014. Have another look then!</h5>

<p>Link Recommender for Drupal 7</p>

<p>Searches the database for title matches that might make for useful links. 
Provides a list of suggested links, and automatically creates links upon user 
approval. Very helpful for the NCpedia project (http://ncpedia.org), perhaps for 
similar projects as well.</p>

<p><strong>Still very much in beta. Back up your database before installing or using.</strong>
Makes use of MySQL LIKE searching, which is very inefficient. Would be better 
implemented as a Solr or other search, but this does work in Drupal without other
dependencies at least.</p>

<p>To-dos:
*Generalize code for easier adoption by other projects (this grew up 
organically, still has the NCpedia label)
*Improve link context
*More robust search options/administrative control
*Unit testing; field testing</p>
