<h4>Link Recommender for Drupal 7</h4>

<p>Creates a "Link Suggestions" tab on node pages. On this tab, a list of 
suggested links is generated for the node. Automatically creates links upon user 
approval. Very helpful for the NCpedia project (http://ncpedia.org), perhaps for 
similar projects as well.</p>

<img src="http://joshwilson.net/img/ncpedia_linkrecs_screenshot.jpg" />

<p><strong>Still very much in beta. Back up your database before installing or using.</strong>
Default configuration makes use of MySQL LIKE searching, which is very inefficient. Also supports
MySQL FULLTEXT searching (not fully documented as of April 2014; see TODOs).</p>

<p>To-dos:</p>
<ul>
<li>Generalize code for easier adoption by other projects (this grew up 
organically, still has the NCpedia label)</li>
<li>Complete documentation</li>
<li>testing, testing, testing</li>
</ul>
