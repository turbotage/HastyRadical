#!/usr/bin/env python3
"""
Dash app to visualize Gamma(n) statistics from gamma_n_stat.txt files
"""

import dash
from dash import dcc, html, Input, Output, dash_table
import plotly.express as px
import plotly.graph_objects as go
import pandas as pd
import numpy as np
from typing import List, NamedTuple
import os
import glob

class CheckResult(NamedTuple):
    products_tested: int
    inversion_permutation_at_success: int
    ypos_at_success: int
    gens_giving_success: List[int]
    gens_permutation: List[int]

def deserialize_check_results(filename: str) -> List[CheckResult]:
    """
    Deserialize CheckResult vector from text file.
    """
    results = []
    
    try:
        with open(filename, 'r') as file:
            lines = [line.strip() for line in file.readlines()]
        
        for i in range(0, len(lines), 5):
            if i + 4 >= len(lines):
                break
            
            try:
                products_tested = int(lines[i])
                inversion_permutation = int(lines[i + 1])
                ypos = int(lines[i + 2])
                
                # Parse gens_giving_success vector
                gens_giving_success = []
                if len(lines[i + 3]) > 1:
                    vector_data = lines[i + 3][1:]
                    if vector_data:
                        gens_giving_success = [int(x.strip()) for x in vector_data.split(',')]
                
                # Parse gens_permutation vector
                gens_permutation = []
                if len(lines[i + 4]) > 1:
                    vector_data = lines[i + 4][1:]
                    if vector_data:
                        gens_permutation = [int(x.strip()) for x in vector_data.split(',')]
                
                result = CheckResult(products_tested, inversion_permutation, ypos,
                                   gens_giving_success, gens_permutation)
                results.append(result)
                
            except ValueError as e:
                print(f"Error parsing entry starting at line {i+1}: {e}")
                continue
                
    except FileNotFoundError:
        print(f"File {filename} not found")
        return []
    except Exception as e:
        print(f"Error reading file {filename}: {e}")
        return []
    
    return results

def load_all_gamma_stats():
    """Load all gamma_n_stat.txt files"""
    data = {}
    pattern = "statistics/gamma_*_stat.txt"
    
    # Check if statistics directory exists
    if not os.path.exists("statistics"):
        print("Statistics directory not found!")
        return data
    
    for filename in glob.glob(pattern):
        try:
            # Extract n from filename (e.g., statistics/gamma_5_stat.txt -> 5)
            basename = os.path.basename(filename)
            n = int(basename.split('_')[1])
            results = deserialize_check_results(filename)
            if results:
                data[n] = results
                print(f"Loaded {len(results)} results for Gamma({n})")
        except (ValueError, IndexError):
            print(f"Could not parse n from filename: {filename}")
    
    return data

def create_summary_dataframe(gamma_data):
    """Create summary statistics DataFrame"""
    summary_data = []
    
    for n, results in gamma_data.items():
        if not results:
            continue
            
        products_tested = [r.products_tested for r in results]
        successful_results = [r for r in results if r.products_tested > 0]
        successful_generators = len(successful_results)
        
        # Generator sequence statistics (sequence length = number of generators used in successful product)
        generator_seq_lengths = [len(r.gens_giving_success) for r in successful_results]
        
        # Count generators by sequence length (0, 1, 2, 3+ products)
        seq_length_counts = pd.Series(generator_seq_lengths).value_counts() if generator_seq_lengths else pd.Series()
        generators_with_0 = len(results) - successful_generators  # Failed generators
        generators_with_1 = seq_length_counts.get(1, 0)
        generators_with_2 = seq_length_counts.get(2, 0)
        generators_with_3plus = sum(seq_length_counts.get(i, 0) for i in range(3, max(generator_seq_lengths) + 1)) if generator_seq_lengths else 0
        
        # Count generators by inversion permutation index at success
        inversion_perms = [r.inversion_permutation_at_success for r in successful_results]
        inversion_counts = pd.Series(inversion_perms).value_counts() if inversion_perms else pd.Series()
        perm_index_0 = inversion_counts.get(0, 0)
        perm_index_1 = inversion_counts.get(1, 0)
        perm_index_2 = inversion_counts.get(2, 0)
        perm_index_3 = inversion_counts.get(3, 0)
        perm_index_4plus = sum(inversion_counts.get(i, 0) for i in range(4, max(inversion_perms) + 1)) if inversion_perms else 0
        
        # Count generators by Y position at success
        ypos_values = [r.ypos_at_success for r in successful_results]
        ypos_counts = pd.Series(ypos_values).value_counts() if ypos_values else pd.Series()
        ypos_0 = ypos_counts.get(0, 0)
        ypos_1 = ypos_counts.get(1, 0)
        ypos_2 = ypos_counts.get(2, 0)
        ypos_3 = ypos_counts.get(3, 0)
        
        # Most frequently used generators
        all_gens_used = []
        for r in successful_results:
            all_gens_used.extend(r.gens_giving_success)
        most_used_generator = pd.Series(all_gens_used).mode()[0] if all_gens_used else -1
        
        # Products tested statistics
        median_products_tested = np.median(products_tested) if products_tested else 0
        
        summary_data.append({
            'n': n,
            'total_generators': len(results),
            'successful_generators': successful_generators,
            'gens_with_0': generators_with_0,
            'gens_with_1': generators_with_1,
            'gens_with_2': generators_with_2,
            'gens_with_3plus': generators_with_3plus,
            'perm_index_0': perm_index_0,
            'perm_index_1': perm_index_1,
            'perm_index_2': perm_index_2,
            'perm_index_3': perm_index_3,
            'perm_index_4plus': perm_index_4plus,
            'ypos_0': ypos_0,
            'ypos_1': ypos_1,
            'ypos_2': ypos_2,
            'ypos_3': ypos_3,
            'most_used_gen': most_used_generator,
            'avg_products_tested': np.mean(products_tested) if products_tested else 0,
            'median_products_tested': median_products_tested,
            'max_products_tested': max(products_tested) if products_tested else 0,
            'total_products_tested': sum(products_tested)
        })
    
    df = pd.DataFrame(summary_data)
    if not df.empty:
        return df.sort_values('n')
    else:
        return df

# Initialize Dash app
app = dash.Dash(__name__)

# Load data
gamma_data = load_all_gamma_stats()
summary_df = create_summary_dataframe(gamma_data)

# Load generator matrices
def gamma_isomorphism_np(mat: np.ndarray, n: int) -> np.ndarray:
    """
    Apply the gamma_isomorphism transformation to a 2x2 numpy matrix (in-place).
    mat: shape (2,2), dtype np.int64
    """
    mat = mat.copy()
    mat[0,0] -= 1
    mat[1,1] -= 1
    mat[0,0] //= n
    mat[0,1] //= n
    mat[1,0] //= n
    mat[1,1] //= n
    return mat

def load_generator_matrices():
    """
    Load 2x2 matrices for each generator for each n.
    Expects files: generators/gamma_{n}_generators.txt
    Each generator: 4 lines (x11, x12, x21, x22)
    Applies gamma_isomorphism to each matrix.
    """
    matrices = {}
    pattern = "generators/gamma_*_generators.txt"
    for filename in glob.glob(pattern):
        try:
            basename = os.path.basename(filename)
            n = int(basename.split('_')[1])
            with open(filename, 'r') as f:
                lines = [line.strip() for line in f if line.strip()]
                matrices[n] = []
                for i in range(0, len(lines), 4):
                    if i + 3 >= len(lines):
                        break
                    try:
                        # Use np.int64 for all values
                        x11 = np.int64(lines[i])
                        x12 = np.int64(lines[i+1])
                        x21 = np.int64(lines[i+2])
                        x22 = np.int64(lines[i+3])
                        mat = np.array([[x11, x12], [x21, x22]], dtype=np.int64)
                        mat = gamma_isomorphism_np(mat, n)
                        matrices[n].append(mat)
                    except Exception as e:
                        print(f"Error parsing generator matrix for n={n} at lines {i}-{i+3}: {e}")
        except Exception as e:
            print(f"Error loading generator matrices from {filename}: {e}")
    return matrices

generator_matrices = load_generator_matrices()

if gamma_data:
    print(f"Found data for: {sorted(gamma_data.keys())}")
else:
    print("No gamma_*_stat.txt files found!")

app.layout = html.Div([
    html.H1("Gamma(n) Statistics Dashboard", 
            style={'textAlign': 'center', 'marginBottom': 30}),
    
    # Summary statistics
    html.Div([
        html.H2("Summary Statistics"),
        dash_table.DataTable(
            id='summary-table',
            data=summary_df.to_dict('records') if not summary_df.empty else [],
            columns=[
                {'name': 'n', 'id': 'n'},
                {'name': 'Total Generators', 'id': 'total_generators'},
                {'name': '1 Gen Prod', 'id': 'gens_with_1'},
                {'name': '2 Gen Prod', 'id': 'gens_with_2'},
                {'name': '3+ Gen Prod', 'id': 'gens_with_3plus'},
                {'name': 'Perm 0', 'id': 'perm_index_0'},
                {'name': 'Perm 1', 'id': 'perm_index_1'},
                {'name': 'Perm 2', 'id': 'perm_index_2'},
                {'name': 'Perm 3', 'id': 'perm_index_3'},
                {'name': 'Perm 4+', 'id': 'perm_index_4plus'},
                {'name': 'YPos 0', 'id': 'ypos_0'},
                {'name': 'YPos 1', 'id': 'ypos_1'},
                {'name': 'YPos 2', 'id': 'ypos_2'},
                {'name': 'YPos 3', 'id': 'ypos_3'},
                {'name': 'Avg Products Tested', 'id': 'avg_products_tested', 'type': 'numeric', 'format': {'specifier': '.1f'}},
                {'name': 'Median Products Tested', 'id': 'median_products_tested', 'type': 'numeric', 'format': {'specifier': '.1f'}},
                {'name': 'Total Products Tested', 'id': 'total_products_tested', 'type': 'numeric', 'format': {'specifier': ','}}
            ],
            sort_action='native',
            style_cell={'textAlign': 'center'},
            style_header={'backgroundColor': 'rgb(230, 230, 230)', 'fontWeight': 'bold'}
        ) if not summary_df.empty else html.P("No data files found. Generate gamma_*_stat.txt files first."),
        
        html.Div([
            html.P("Perm X = Generators that succeeded at inversion permutation index X; X Gen Prod = Generators that succeeded using X generators in the product", 
                   style={'fontSize': '12px', 'fontStyle': 'italic', 'color': 'gray', 'margin': '2px 0'}),
            html.P("* YPos X = Number of generators that succeeded at Y position X", 
                   style={'fontSize': '12px', 'fontStyle': 'italic', 'color': 'gray', 'margin': '2px 0'})
        ], style={'marginTop': '5px'})
    ], style={'marginBottom': 30}),
    
    # Selector for specific n value
    html.Div([
        html.Label('Select Gamma(n) to analyze in detail:'),
        dcc.Dropdown(
            id='n-selector',
            options=[{'label': f'Gamma({n})', 'value': n} for n in sorted(gamma_data.keys())],
            value=min(gamma_data.keys()) if gamma_data else None,
            style={'width': '300px'}
        )
    ], style={'marginBottom': 20}),
    # --- NEW: Generator matrix viewer ---
    html.Div([
        html.Label('Enter Generator Index to view its 2x2 matrix:'),
        dcc.Input(
            id='matrix-generator-index-input',
            type='number',
            placeholder='Generator index',
            style={'width': '120px', 'marginLeft': '10px'}
        ),
        html.Div(id='generator-matrix-display', style={'marginTop': '10px'})
    ], style={'marginBottom': 30}),
    
    # Detailed analysis for selected n
    html.Div(id='detailed-analysis'),
    
    # Generator Dependency Network
    html.Div([
        html.H2("Generator Dependency Network"),
        html.P("Interactive network showing computational dependencies between generators. Click on a node to see details."),
        dcc.Graph(id='generator-network')
    ], style={'marginBottom': 30}),
    
    # Selected generator details
    html.Div(id='selected-generator-details'),
    
    # Generator Dependency Tree
    html.Div([
        html.H2("Generator Dependency Tree"),
        html.P("Trace the computational lineage of a specific generator backwards through its dependencies."),
        html.Div([
            html.Label('Select Generator Index:'),
            dcc.Input(
                id='generator-index-input',
                type='number',
                placeholder='Enter generator index',
                style={'width': '200px', 'marginLeft': '10px'}
            )
        ], style={'marginBottom': 20}),
        dcc.Graph(id='generator-tree')
    ], style={'marginBottom': 30}),
    
    # Overall trends
    html.Div([
        html.H2("Generator Usage and Permutation Patterns"),
        
        html.Div([
            dcc.Graph(id='generator-usage-trend'),
            dcc.Graph(id='products-tested-trend')
        ], style={'display': 'flex'}),
        
        html.Div([
            dcc.Graph(id='generator-count-trend'),
            dcc.Graph(id='permutation-patterns-trend')
        ], style={'display': 'flex'})
    ])
])

def create_top_generators_table(results, selected_n):
    """Create a table showing the top 50 most computationally expensive generators"""
    if not results:
        return html.P("No data available")
    
    # Create a list of (generator_index, result) pairs for successful results
    successful_results_with_index = [(i, r) for i, r in enumerate(results) if r.products_tested > 0]
    if not successful_results_with_index:
        return html.P("No successful generators found")
    
    # Sort by products tested and take top 50
    top_generators = sorted(successful_results_with_index, key=lambda x: x[1].products_tested, reverse=True)[:50]
    
    # Create data for the table
    table_data = []
    for rank, (generator_index, result) in enumerate(top_generators, 1):
        # Format the generator sequence
        gen_sequence = ', '.join(map(str, result.gens_giving_success)) if result.gens_giving_success else "N/A"
        
        table_data.append({
            'rank': rank,
            'generator_index': generator_index,
            'products_tested': result.products_tested,
            'generator_sequence': gen_sequence,
            'sequence_length': len(result.gens_giving_success),
            'ypos': result.ypos_at_success,
            'inv_perm': result.inversion_permutation_at_success,
            'gen_perm': ', '.join(map(str, result.gens_permutation)) if result.gens_permutation else "N/A"
        })
    
    return dash_table.DataTable(
        data=table_data,
        columns=[
            {'name': 'Rank', 'id': 'rank'},
            {'name': 'Generator Index', 'id': 'generator_index'},
            {'name': 'Products Tested', 'id': 'products_tested'},
            {'name': 'Generator Sequence', 'id': 'generator_sequence'},
            {'name': 'Seq Length', 'id': 'sequence_length'},
            {'name': 'Y Position', 'id': 'ypos'},
            {'name': 'Inv Permutation', 'id': 'inv_perm'},
            {'name': 'Generator Permutation', 'id': 'gen_perm'}
        ],
        style_cell={
            'textAlign': 'center',
            'padding': '8px',
            'fontSize': '12px'
        },
        style_header={
            'backgroundColor': 'rgb(200, 220, 240)',
            'fontWeight': 'bold',
            'fontSize': '12px'
        },
        style_data_conditional=[
            {
                'if': {'row_index': 0},
                'backgroundColor': 'rgb(255, 240, 240)',
                'color': 'black',
            }
        ],
        style_table={'overflowX': 'auto'}
    )

@app.callback(
    Output('detailed-analysis', 'children'),
    Input('n-selector', 'value')
)
def update_detailed_analysis(selected_n):
    if selected_n is None or selected_n not in gamma_data:
        return html.Div("No data available")
    
    results = gamma_data[selected_n]
    
    # Create detailed plots
    products_tested = [r.products_tested for r in results]
    inversion_perms = [r.inversion_permutation_at_success for r in results if r.products_tested > 0]
    ypos_values = [r.ypos_at_success for r in results if r.products_tested > 0]
    
    # Products tested histogram
    products_hist = px.histogram(
        x=products_tested,
        nbins=50,
        title=f"Distribution of Products Tested - Gamma({selected_n})",
        labels={'x': 'Products Tested', 'y': 'Frequency'}
    )
    
    # Inversion permutation distribution
    inversion_hist = px.histogram(
        x=inversion_perms,
        title=f"Inversion Permutation at Success - Gamma({selected_n})",
        labels={'x': 'Inversion Permutation', 'y': 'Frequency'}
    ) if inversion_perms else go.Figure().add_annotation(text="No successful results")
    
    # Y position distribution
    ypos_hist = px.histogram(
        x=ypos_values,
        title=f"Y Position at Success - Gamma({selected_n})",
        labels={'x': 'Y Position', 'y': 'Frequency'}
    ) if ypos_values else go.Figure().add_annotation(text="No successful results")
    
    # Generator usage analysis
    all_gens_used = []
    generator_sequence_lengths = []
    successful_results = [r for r in results if r.products_tested > 0]
    
    for r in successful_results:
        all_gens_used.extend(r.gens_giving_success)
        generator_sequence_lengths.append(len(r.gens_giving_success))
    
    if all_gens_used:
        gen_usage_counts = pd.Series(all_gens_used).value_counts().sort_index()
        gen_usage_plot = px.bar(
            x=gen_usage_counts.index,
            y=gen_usage_counts.values,
            title=f"Generator Usage Frequency - Gamma({selected_n})",
            labels={'x': 'Generator Index', 'y': 'Usage Count'}
        )
    else:
        gen_usage_plot = go.Figure().add_annotation(text="No generator usage data")
    
    # Generator sequence length distribution
    if generator_sequence_lengths:
        seq_length_hist = px.histogram(
            x=generator_sequence_lengths,
            title=f"Distribution of Generator Sequence Lengths - Gamma({selected_n})",
            labels={'x': 'Number of Generators in Sequence', 'y': 'Frequency'},
            nbins=max(1, max(generator_sequence_lengths) - min(generator_sequence_lengths) + 1)
        )
    else:
        seq_length_hist = go.Figure().add_annotation(text="No sequence length data")
    
    # Calculate more detailed statistics
    avg_seq_length = np.mean(generator_sequence_lengths) if generator_sequence_lengths else 0
    most_common_seq_length = pd.Series(generator_sequence_lengths).mode()[0] if generator_sequence_lengths else 0
    
    return html.Div([
        html.H2(f"Detailed Analysis for Gamma({selected_n})"),
        
        html.Div([
            html.P(f"Total generators: {len(results)}"),
            html.P(f"Successful generators: {len(successful_results)}"),
            html.P(f"Average generator sequence length: {avg_seq_length:.1f}"),
            html.P(f"Most common sequence length: {most_common_seq_length}"),
            html.P(f"Average products tested: {np.mean(products_tested):.1f}"),
            html.P(f"Total computation effort: {sum(products_tested):,} products tested")
        ], style={'backgroundColor': '#f0f0f0', 'padding': '10px', 'marginBottom': '20px'}),
        
        # Full-width generator usage frequency chart
        html.Div([
            dcc.Graph(figure=gen_usage_plot)
        ], style={'width': '100%', 'marginBottom': '20px'}),
        
        html.Div([
            dcc.Graph(figure=seq_length_hist),
            dcc.Graph(figure=inversion_hist)
        ], style={'display': 'flex'}),
        
        html.Div([
            dcc.Graph(figure=ypos_hist)
        ], style={'display': 'flex', 'justifyContent': 'center'}),
        
        # Top 100 most computationally expensive generators
        html.Div([
            html.H3("Top 100 Most Computationally Expensive Generators"),
            create_top_generators_table(results, selected_n)
        ], style={'marginTop': '30px'})
    ])

@app.callback(
    [Output('generator-usage-trend', 'figure'),
     Output('products-tested-trend', 'figure'),
     Output('generator-count-trend', 'figure'),
     Output('permutation-patterns-trend', 'figure')],
    Input('n-selector', 'value')  # Dummy input to trigger update
)
def update_trend_plots(_):
    if summary_df.empty:
        empty_fig = go.Figure().add_annotation(text="No data available")
        return empty_fig, empty_fig, empty_fig, empty_fig
    
    # Generator usage patterns across all n
    generator_lengths = []
    n_values = []
    for n, results in gamma_data.items():
        for result in results:
            if result.products_tested > 0:  # Only successful results
                generator_lengths.append(len(result.gens_giving_success))
                n_values.append(n)
    
    if generator_lengths:
        gen_length_fig = px.box(
            x=n_values, y=generator_lengths,
            title='Number of Generators Used in Successful Products vs n',
            labels={'x': 'n', 'y': 'Number of Generators Used'}
        )
    else:
        gen_length_fig = go.Figure().add_annotation(text="No successful results found")
    
    # Products tested trend (computational effort)
    products_fig = px.scatter(
        summary_df, x='n', y='avg_products_tested',
        size='total_generators',
        title='Average Products Tested vs n',
        labels={'avg_products_tested': 'Avg Products Tested', 'n': 'n'}
    )
    
    # Generator count trend
    generators_fig = px.bar(
        summary_df, x='n', y='total_generators',
        title='Total Generators vs n',
        labels={'total_generators': 'Total Generators', 'n': 'n'}
    )
    
    # Permutation analysis - distribution of inversion permutations
    inversion_perms = []
    n_vals_inv = []
    for n, results in gamma_data.items():
        for result in results:
            if result.products_tested > 0:
                inversion_perms.append(result.inversion_permutation_at_success)
                n_vals_inv.append(n)
    
    if inversion_perms:
        inversion_fig = px.box(
            x=n_vals_inv, y=inversion_perms,
            title='Inversion Permutation Patterns vs n',
            labels={'x': 'n', 'y': 'Inversion Permutation at Success'}
        )
    else:
        inversion_fig = go.Figure().add_annotation(text="No inversion data found")
    
    return gen_length_fig, products_fig, generators_fig, inversion_fig

@app.callback(
    Output('generator-network', 'figure'),
    Input('n-selector', 'value')
)
def update_generator_network(selected_n):
    if selected_n is None or selected_n not in gamma_data:
        return go.Figure().add_annotation(text="No data available")
    
    results = gamma_data[selected_n]
    successful_results = [(i, r) for i, r in enumerate(results) if r.products_tested > 0]
    
    if not successful_results:
        return go.Figure().add_annotation(text="No successful generators found")
    
    # Limit to top 100 for better performance and visibility
    successful_results = successful_results[:100]
    num_nodes = len(successful_results)
    
    if num_nodes == 0:
        return go.Figure().add_annotation(text="No successful generators found")
    
    # Prepare data lists
    node_x = []
    node_y = []
    node_text = []
    node_color = []
    node_hover = []
    edge_x = []
    edge_y = []
    
    # Position nodes in a better grid layout instead of circular
    import math
    cols = math.ceil(math.sqrt(num_nodes))
    rows = math.ceil(num_nodes / cols)
    
    for i, (gen_idx, result) in enumerate(successful_results):
        row = i // cols
        col = i % cols
        x = col * 2 - cols + 1  # Center the grid
        y = rows - row - 1      # Flip y to have origin at bottom
        
        node_x.append(x)
        node_y.append(y)
        node_text.append(str(gen_idx))
        node_color.append(result.products_tested)
        
        gen_sequence = ', '.join(map(str, result.gens_giving_success)) if result.gens_giving_success else "N/A"
        hover_info = (f"Generator {gen_idx}<br>"
                     f"Products Tested: {result.products_tested}<br>"
                     f"Y Position: {result.ypos_at_success}<br>"
                     f"Inv Permutation: {result.inversion_permutation_at_success}<br>"
                     f"Uses Generators: {gen_sequence}")
        node_hover.append(hover_info)
        
        # Create edges based on generator dependencies
        for other_gen_idx in result.gens_giving_success:
            # Find if this referenced generator is also in our successful results
            for j, (other_idx, _) in enumerate(successful_results):
                if other_idx == other_gen_idx:
                    other_row = j // cols
                    other_col = j % cols
                    other_x = other_col * 2 - cols + 1
                    other_y = rows - other_row - 1
                    
                    edge_x.extend([x, other_x, None])
                    edge_y.extend([y, other_y, None])
                    break
    
    # Create edge trace
    edge_trace = go.Scatter(
        x=edge_x, y=edge_y,
        line=dict(width=0.5, color='#888'),
        hoverinfo='none',
        mode='lines'
    )
    
    # Create node trace
    node_trace = go.Scatter(
        x=node_x, y=node_y,
        mode='markers+text',
        hoverinfo='text',
        hovertext=node_hover,
        text=node_text,
        textposition="middle center",
        marker=dict(
            showscale=True,
            colorscale='Viridis',
            reversescale=True,
            color=node_color,
            size=25,  # Larger nodes
            colorbar=dict(
                thickness=15,
                len=0.5,
                x=0.85,
                title="Products Tested"
            ),
            line=dict(width=2)
        )
    )
    
    fig = go.Figure(data=[edge_trace, node_trace],
                    layout=go.Layout(
                        title=dict(
                            text=f'Generator Dependency Network - Gamma({selected_n}) (Top 100)',
                            font=dict(size=16)
                        ),
                        showlegend=False,
                        hovermode='closest',
                        margin=dict(b=20,l=5,r=5,t=40),
                        annotations=[dict(
                            text="Click on nodes to see details. Edges show dependencies.",
                            showarrow=False,
                            xref="paper", yref="paper",
                            x=0.005, y=-0.002,
                            xanchor="left", yanchor="bottom",
                            font=dict(color="#888", size=10)
                        )],
                        xaxis=dict(showgrid=False, zeroline=False, showticklabels=False),
                        yaxis=dict(showgrid=False, zeroline=False, showticklabels=False)
                    ))
    
    return fig

@app.callback(
    Output('generator-tree', 'figure'),
    [Input('generator-index-input', 'value'),
     Input('n-selector', 'value')]
)
def update_generator_tree(generator_index, selected_n):
    if selected_n is None or selected_n not in gamma_data or generator_index is None:
        return go.Figure().add_annotation(text="Select a Gamma(n) and enter a generator index")
    
    results = gamma_data[selected_n]
    
    if generator_index < 0 or generator_index >= len(results):
        return go.Figure().add_annotation(text=f"Generator index must be between 0 and {len(results)-1}")
    
    # Build the dependency tree recursively
    def build_dependency_tree(gen_idx, level=0, visited=None):
        if visited is None:
            visited = set()
        
        if gen_idx in visited or gen_idx >= len(results):
            return []
        
        visited.add(gen_idx)
        result = results[gen_idx]
        
        # Only include if this generator was successful
        if result.products_tested == 0:
            return []
        
        node_info = {
            'gen_idx': gen_idx,
            'level': level,
            'result': result,
            'children': []
        }
        
        # Add children (generators this one depends on)
        for child_idx in result.gens_giving_success:
            if child_idx < len(results):
                child_nodes = build_dependency_tree(child_idx, level + 1, visited.copy())
                node_info['children'].extend(child_nodes)
        
        return [node_info] + node_info['children']
    
    # Check if the selected generator exists and is successful
    if results[generator_index].products_tested == 0:
        return go.Figure().add_annotation(text=f"Generator {generator_index} was not successfully solved")
    
    # Initialize data structures
    node_x = []
    node_y = []
    node_text = []
    node_color = []
    node_hover = []
    edge_x = []
    edge_y = []
    
    # Count nodes per level for better spacing (we'll do this in the recursive function)
    tree_nodes = build_dependency_tree(generator_index)
    level_counts = {}
    for node in tree_nodes:
        level = node['level']
        level_counts[level] = level_counts.get(level, 0) + 1
    
    # Create proper parent-child connections
    # First, rebuild the tree with proper parent-child relationships
    def build_tree_with_connections(gen_idx, level=0, visited=None, parent_pos=None):
        if visited is None:
            visited = set()
        
        if gen_idx in visited or gen_idx >= len(results):
            return []
        
        visited.add(gen_idx)
        result = results[gen_idx]
        
        if result.products_tested == 0:
            return []
        
        # Position this node
        if level not in level_positions:
            level_positions[level] = 0
        
        y = -level * 3  # Levels go downward with more spacing
        x = (level_positions[level] - level_counts[level]/2) * 4  # More horizontal spacing
        level_positions[level] += 1
        
        current_pos = len(node_x)
        node_x.append(x)
        node_y.append(y)
        node_text.append(str(gen_idx))
        
        # Use log scale for better color distribution
        log_products = np.log10(max(1, result.products_tested))
        node_color.append(log_products)
        
        gen_sequence = ', '.join(map(str, result.gens_giving_success)) if result.gens_giving_success else "N/A"
        hover_info = (f"Generator {gen_idx} (Level {level})<br>"
                     f"Products Tested: {result.products_tested}<br>"
                     f"Y Position: {result.ypos_at_success}<br>"
                     f"Inv Permutation: {result.inversion_permutation_at_success}<br>"
                     f"Uses Generators: {gen_sequence}")
        node_hover.append(hover_info)
        
        # Connect to parent if it exists
        if parent_pos is not None:
            edge_x.extend([node_x[parent_pos], x, None])
            edge_y.extend([node_y[parent_pos], y, None])
        
        # Process children recursively
        for child_idx in result.gens_giving_success:
            if child_idx < len(results):
                build_tree_with_connections(child_idx, level + 1, visited.copy(), current_pos)
    
    # Clear the previous positioning data and rebuild properly
    level_positions = {}
    build_tree_with_connections(generator_index)
    
    # Create edge trace
    edge_trace = go.Scatter(
        x=edge_x, y=edge_y,
        line=dict(width=2, color='#888'),
        hoverinfo='none',
        mode='lines'
    )
    
    # Create node trace
    node_trace = go.Scatter(
        x=node_x, y=node_y,
        mode='markers+text',
        hoverinfo='text',
        hovertext=node_hover,
        text=node_text,
        textposition="middle center",
        marker=dict(
            showscale=True,
            colorscale='Viridis',  # Better color scale
            color=node_color,
            size=35,  # Slightly larger nodes
            colorbar=dict(
                thickness=15,
                len=0.5,
                x=0.85,
                title="log₁₀(Products Tested)"
            ),
            line=dict(width=2, color='white')  # White border for better visibility
        )
    )
    
    fig = go.Figure(data=[edge_trace, node_trace],
                    layout=go.Layout(
                        title=dict(
                            text=f'Dependency Tree for Generator {generator_index} - Gamma({selected_n})',
                            font=dict(size=16)
                        ),
                        showlegend=False,
                        hovermode='closest',
                        margin=dict(b=20,l=5,r=5,t=40),
                        xaxis=dict(showgrid=False, zeroline=False, showticklabels=False),
                        yaxis=dict(showgrid=False, zeroline=False, showticklabels=False)
                    ))
    
    return fig

@app.callback(
    Output('selected-generator-details', 'children'),
    Input('generator-network', 'clickData'),
    Input('n-selector', 'value')
)
def display_click_data(clickData, selected_n):
    if clickData is None or selected_n is None or selected_n not in gamma_data:
        return html.Div()
    
    try:
        point_index = clickData['points'][0]['pointIndex']
        results = gamma_data[selected_n] 
        successful_results = [(i, r) for i, r in enumerate(results) if r.products_tested > 0]
        
        if point_index < len(successful_results):
            gen_idx, result = successful_results[point_index]
            gen_sequence = ', '.join(map(str, result.gens_giving_success)) if result.gens_giving_success else "N/A"
            gen_perm = ', '.join(map(str, result.gens_permutation)) if result.gens_permutation else "N/A"
            
            return html.Div([
                html.H3(f"Selected Generator {gen_idx}"),
                html.Div([
                    html.P(f"Products Tested: {result.products_tested}"),
                    html.P(f"Y Position at Success: {result.ypos_at_success}"),
                    html.P(f"Inversion Permutation: {result.inversion_permutation_at_success}"),
                    html.P(f"Generator Sequence Used: {gen_sequence}"),
                    html.P(f"Generator Permutation: {gen_perm}"),
                    html.P(f"Sequence Length: {len(result.gens_giving_success)}")
                ], style={'backgroundColor': '#f0f0f0', 'padding': '15px', 'borderRadius': '5px'})
            ], style={'marginTop': '20px', 'marginBottom': '20px'})
    except (KeyError, IndexError):
        pass
    
    return html.Div()

@app.callback(
    Output('generator-matrix-display', 'children'),
    [Input('matrix-generator-index-input', 'value'),
     Input('n-selector', 'value')]
)
def show_generator_matrix(generator_index, selected_n):
    if selected_n is None or generator_index is None:
        return html.Div()
    if selected_n not in generator_matrices:
        return html.Div("No generator matrix data for this n.")
    matrices = generator_matrices[selected_n]
    if generator_index < 0 or generator_index >= len(matrices):
        return html.Div(f"Generator index must be between 0 and {len(matrices)-1}")
    mat = matrices[generator_index]
    # Format as a table
    return html.Table([
        html.Tr([html.Td(str(mat[0,0])), html.Td(str(mat[0,1]))]),
        html.Tr([html.Td(str(mat[1,0])), html.Td(str(mat[1,1]))])
    ], style={'border': '1px solid black', 'marginTop': '5px', 'fontSize': '18px', 'textAlign': 'center'})

if __name__ == '__main__':
    if not gamma_data:
        print("No gamma_*_stat.txt files found in current directory!")
        print("Make sure to run your C++ program to generate the statistics files first.")
    else:
        port = int(os.getenv('PORT', '8051'))
        print(f"Loaded data for Gamma(n) where n = {sorted(gamma_data.keys())}")
        print(f"Starting Dash app on http://127.0.0.1:{port} (and accessible on your LAN at http://<your-ip>:{port})")
    
    # Bind to all interfaces so it's reachable outside localhost; allow overriding port via PORT env var
    app.run(host='0.0.0.0', port=int(os.getenv('PORT', '8051')), debug=True)
