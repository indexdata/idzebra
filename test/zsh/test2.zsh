quickstart
init 
create_database Default
record_insert <xml><head>foo</head><body>bar</body></xml>
search_pqf firstset @attr 1=/ nothere
expect 0 hits
search_pqf firstset @attr 1=/ foo
expect 1 hits
